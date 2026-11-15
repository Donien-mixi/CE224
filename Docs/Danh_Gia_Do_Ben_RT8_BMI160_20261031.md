# Đánh giá độ bền RT8/NET5 khi va đập, lỗi BMI160 và mất dữ liệu STM32

Ngày 31/10/2026. Nền source: commit `b95cd71`, STM32 `BAL-20261024-R12-8DIR-RT8`, ESP32 `TWIP-WEB-20261024-BAL-R12-8DIR-RT8-NET5`.

Đây là kết quả đọc mã và nghiên cứu tài liệu, chưa phải kết quả thử lỗi trên phần cứng. Không sửa firmware, không nạp xe, không merge nhánh PipeLine và không thay gain/giới hạn R12 trong lượt nghiên cứu này.

## 1. Hiện tượng đã được người dùng xác nhận

- Firmware mới hoạt động tốt trong điều kiện sử dụng thông thường.
- Sau cú ngã mạnh có thể báo lỗi đọc BMI160, hoặc web báo STM32 chưa kết nối/chưa có dữ liệu mới.
- Wi-Fi vẫn còn; cần rút pin rồi cắm lại để trở về bình thường.

Wi-Fi còn hoạt động không đủ chứng minh ESP32 chưa reset hoặc nguồn STM32/BMI160 không bị sụt. Tuy nhiên, hiện tượng ưu tiên điều tra phía STM32, bus IMU, nguồn cục bộ và UART trước khi quy cho điện thoại hay Wi-Fi. Chưa có TXT/JSON/oscilloscope của đúng lượt lỗi để kết luận nguyên nhân gốc.

Mục tiêu cần tách rõ: (a) giữ thăng bằng trước nhiễu trong giới hạn vật lý, (b) khi đã vượt giới hạn thì dừng an toàn và tiếp tục chẩn đoán, (c) khôi phục cảm biến/giao tiếp sau lỗi có thể khôi phục mà không phải rút pin. Không yêu cầu xe tiếp tục điều khiển motor bằng IMU hỏng.

## 2. Các phát hiện trực tiếp trong source

### 2.1. Timeout truyền vào 2 ms không chặn mọi đường chờ ở 2 ms

`Core/Src/bmi160.c:35` gọi `HAL_I2C_Mem_Read(..., 2)`. Trong bản HAL đang dùng, `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c:2637` kiểm tra bus BUSY bằng `I2C_TIMEOUT_BUSY_FLAG`, được định nghĩa là **25 ms**, trước khi dùng timeout của người gọi cho giao dịch.

Vòng TIM4 có chu kỳ **5 ms**, và `robot_fsm.c:675` gọi đọc BMI160 ngay trong ngắt. Nếu BUSY bị giữ liên tục, một lượt chờ có thể dài hơn nhiều chu kỳ TIM4. Ngắt timer có thể liên tục pending và lấy phần lớn thời gian CPU; `ESP32_Comm_Process()` và `Robot_BalanceService()` trong main không có đủ thời gian phục vụ. Đây là **giả thuyết có cơ sở tĩnh**, chưa đo trên xe.

Chuỗi có thể xảy ra:

```text
Va đập / chập chờn nguồn hoặc tín hiệu
  -> bus I2C không rảnh
  -> chờ BUSY trong ngắt điều khiển vượt chu kỳ
  -> foreground UART/telemetry bị đói thời gian
  -> ESP32 không nhận frame balance hợp lệ đủ mới
  -> web báo STM32 mất dữ liệu dù Wi-Fi vẫn còn
```

Không mặc định đây là deadlock vô hạn: `Robot_Init()` thực tế đặt SysTick priority 0, TIM4 priority 1 (`robot_fsm.c:518–519`), SysTick vẫn tăng được trong ngắt TIM4. Chỉ đọc priority mặc định trong `tim.c` hoặc HAL config sẽ dẫn tới kết luận sai.

### 2.2. Khi đã lỗi, firmware vẫn tiếp tục đọc IMU trong mỗi vòng timer

`robot_fsm.c:676`: đọc thất bại dẫn tới fault 32, dừng motor và trạng thái EMERGENCY. Đây là bảo vệ cần giữ. Tuy nhiên vòng sau vẫn đi vào lần đọc blocking mới, kể cả khi đang EMERGENCY. Guard `execution > 4000 us` nằm sau giao dịch và chỉ áp dụng khi còn BALANCING/BENCH_TEST; nó không ngắt ngang lần chờ đang diễn ra và không tự giải phóng bus.

Telemetry dùng queue IRQ → main, 32 slot, phát frame balance mỗi 4 vòng. Nếu main bị đói thời gian, queue đầy hoặc TX không được phục vụ, CRC/logic web tốt vẫn không tạo ra được dữ liệu mới.

### 2.3. Thông báo web có nghĩa là dữ liệu cũ, không xác định loại hỏng

`ESP32_S3_Gateway/balance_web.h:42` coi frame mới khi tuổi dữ liệu <500 ms. Trạng thái I2C fault và thông báo stale là hai bằng chứng khác nhau. Mất frame có thể đến từ STM32 kẹt, reset, UART/TX DMA lỗi, dây UART, hoặc ESP32 nhận nhưng frame không qua CRC. Không suy ra ngay STM32 mất nguồn hay BMI160 hỏng vật lý.

### 2.4. Phục hồi IMU hiện còn hạn chế

- `BMI160_Init()` có reset peripheral khi tìm chip lúc khởi động, nhưng chưa có FSM phục hồi bus trong vận hành.
- Những lệnh ghi reset/chế độ/range/ODR trong `BMI160_Init()` chưa kiểm tra từng giá trị bool trả về; xác nhận config có ở bước sau, nhưng lỗi từng công đoạn chưa được lưu.
- `Robot_RecalibrateIMU()` dừng timer rồi đọc/hiệu chuẩn; không gọi lại `BMI160_Init()`, không thực hiện bus clear. Đo lại gyro không tương đương phục hồi sensor bị reset, đang suspend hoặc đang giữ SDA.
- Các frame sau lần đọc thất bại có thể còn mang raw lần đọc thành công trước qua `BMI160_GetLastRaw()`. Hiện chưa có trường sample-valid/tuổi mẫu IMU riêng để phân biệt đầy đủ.
- ODR 100 Hz và điều khiển 200 Hz có thể đọc trùng mẫu bình thường. Không dùng hai mẫu raw bằng nhau làm bằng chứng cảm biến đứng; sensortime tăng cũng không tự chứng minh có mẫu accel/gyro mới.

### 2.5. Khả năng giữ motor an toàn khi MCU thật sự treo cần kiểm chứng

STM32 chưa bật IWDG trong HAL config. `HardFault_Handler()` hiện vòng lặp vô hạn, chưa có bản ghi fault hoặc thao tác đưa đầu ra về trạng thái an toàn trong handler. Timer PWM có thể tiếp tục phát giá trị đã lập trình nếu CPU dừng mà ngoại vi còn chạy. Đây là rủi ro từ cấu trúc mã, không phải bằng chứng cú ngã đã gây HardFault.

`Motor_Stop()` hiện đặt hai đầu vào mỗi cầu H HIGH để phanh; khác với `Motor_Coast()` đặt LOW. Không đánh đồng PWM hiển thị bằng 0, motor không được cấp lực chủ động và cầu H đã bị vô hiệu hóa. Cần xác định hành vi điện thực tế khi CPU reset, treo hoặc pin sụt.

Các test controller hiện có mock `BMI160_Read_All()`/`BMI160_Init()`, không chạy giao dịch I2C vật lý hay tất cả đường chờ HAL. Do đó test/replay đã PASS không loại được các rủi ro trên.

## 3. Tài liệu đã đối chiếu và ý nghĩa với xe

| Nguồn chính | Nội dung dùng trong đánh giá | Áp dụng và giới hạn |
|---|---|---|
| [Bosch BMI160 datasheet, Rev 1.0](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf) | ERR_REG, PMU_STATUS, STATUS/data-ready, sensortime, reset và lưu ý va đập | Dùng kiểm tra sensor sống và cấu hình đúng; không suy ra lỗi I2C chỉ do vượt ±8 g. Giới hạn dải đo khác giới hạn chịu sốc cơ học. Va đập rất mạnh vẫn có thể gây hỏng cơ khí. |
| [NXP UM10204, mục 3.1.16](https://www.nxp.com/docs/en/user-guide/UM10204.pdf) | SDA bị giữ LOW: thử 9 xung clock; SCL bị giữ LOW cần reset thiết bị hoặc power cycle nếu không có reset | Bus clear chỉ làm khi SCL thực sự lên được, dùng open-drain; không ép HIGH push-pull. Không hứa mọi bus kẹt đều phục hồi bằng 9 xung. |
| [ST ES0287, STM32F411xC/xE Rev 6](https://www.st.com/resource/en/errata_sheet/es0287-stm32f411xcxe-device-errata-stmicroelectronics.pdf) | §2.9.1 BERR giả trong controller mode, §2.9.3 misplaced STOP cần SWRST | Đọc lỗi cụ thể và revision chip. Không chép workaround analog-filter BUSY của dòng STM32 khác rồi khẳng định áp dụng cho F411. |
| [TI SLVA689A, I2C pull-up](https://www.ti.com/lit/pdf/slva689) | Điện trở kéo lên phụ thuộc khả năng kéo LOW, điện dung bus và rise time | Đo pull-up có sẵn trên GY-BMI160; không mặc định GPIO_PULLUP nội bộ đủ cho 400 kHz. Không thêm điện trở song song vô điều kiện. |
| [ST AN1709, Rev 5](https://www.st.com/resource/en/application_note/an1709-emc-design-guide-for-stm8-stm32-and-legacy-mcus-stmicroelectronics.pdf) | Chống nhiễu cần phối hợp nguồn, PCB và firmware | Cố định dây/giắc, đo nguồn tại tải, kiểm đường hồi dòng; không chỉ thêm tụ rồi coi đã giải quyết. |
| [Espressif ESP32-S3 hardware guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html) | Nguồn, tụ tại đầu cấp và reset | Chỉ dẫn chip phải phân biệt với module đã có LDO/tụ. Đo cả đầu 5 V và 3,3 V, không lấy số đo tĩnh ở XL4015 thay cho xung tại module. |
| [Espressif fatal errors](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/fatal-errors.html) | Brownout có thể reset; thông báo có thể bị cắt khi áp sụt nhanh | Wi-Fi đang có không loại được reset nhanh trước đó; đối chiếu boot_id/reset_reason. Không tắt brownout để che hiện tượng. |
| [Espressif watchdogs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/wdts.html) | Giám sát ngắt/tác vụ có mục tiêu khác nhau | Watchdog là lớp cuối, không thay bus recovery hoặc đo nguồn. Không tăng timeout tùy tiện. |
| [Allegro A4950 datasheet](https://www.allegromicro.com/-/media/files/datasheets/a4950-datasheet.pdf) | Truth table brake/coast, tụ nguồn tải, thermal/overcurrent/UVLO | Cần kiểm motor phanh, trạng thái chân IN khi MCU reset và nhiệt; bảo vệ của driver không tự cung cấp chẩn đoán chi tiết qua UART. |
| [TI SLVA959B, motor-driver PCB layout](https://www.ti.com/lit/an/slva959b/slva959b.pdf) | Dòng hồi, tụ bulk/bypass và bố trí công suất | GND chung nhưng tránh đoạn đường hồi motor đi qua mốc IMU; vị trí tụ quan trọng, không chỉ số µF. |
| [Albert et al., constrained WIP trajectories, 2019](https://arxiv.org/abs/1811.12819) | Mô hình và lập quỹ đạo có giới hạn trạng thái/đầu vào; kiểm chứng quay gắt | Gợi ý giữ dự trữ lực cân bằng khi chạy/quay, không khuyến nghị chép gain hay đổi R12 đang tốt. |
| [Fici & Ducard, RoA của TWIP có LQR bão hòa, 2026, preprint](https://arxiv.org/abs/2607.07231) | Vùng hút và bất biến dưới nhiễu bị chặn; có mô phỏng/thực nghiệm trên hệ của tác giả | Lý do cần đánh giá theo góc, tốc độ góc, vận tốc và bão hòa. Kết quả số của hệ đó không phải chứng nhận xe này; ở đây chỉ đọc abstract/phạm vi, chưa tái lập chứng minh. |

Các đề xuất dưới đây là tổng hợp cho project, không phải yêu cầu chứng nhận công nghiệp hoặc cam kết chịu mọi cú ngã.

## 4. Thu bằng chứng ở lần lỗi tiếp theo

Kê/giữ xe an toàn, ngừng thử gây ngã tiếp khi lỗi đã xuất hiện. Trước khi rút pin, nếu web còn đáp ứng thì lưu `/network-status` hoặc tải chẩn đoán JSON, `/balance-status`, `/balance-log` và các archive đã hoàn tất. Một số thao tác tải hiện yêu cầu frame mới nên có thể bị chặn lúc stale; ghi nhận cả việc bị chặn, không hứa chắc tải được.

1. Ghi thời điểm xảy ra, đang đứng yên/đi/quay, bề mặt, pin đo được, firmware ID, trạng thái motor và LED/còi sau lỗi.
2. So boot_id/uptime/reset_reason ESP32 trước và sau. Dữ liệu này chỉ đại diện ESP32, không phải reset reason STM32.
3. Máy hiện sóng: đo 3,3 V tại BMI160 và STM32, 5 V tại ESP32; thêm VM nếu đủ kênh. Dùng trigger single-shot để thấy cả sụt và vọt áp. Máy phân tích logic chỉ cho logic, không thay phép đo điện áp analog.
4. Quan sát PB8/PB9: SDA LOW hay SCL LOW kéo dài, cả hai HIGH nhưng BUSY trong peripheral, NACK hay giao dịch có lỗi thời gian.
5. Quan sát PB6 TX STM32 và GPIO18 RX ESP32: không còn byte từ nguồn, byte lỗi, hay byte còn đến nhưng không nhận frame hợp lệ. Đo ở cả hai đầu giúp phân biệt dây/nguồn/gateway.
6. Nếu có ST-LINK: halt sau khi xe đã an toàn, lưu PC/LR/xPSR, I2C SR1/SR2/CR1, State/ErrorCode, pending TIM4 và queue counters. Dừng debug làm đổi thời gian hệ thống, không dùng số đo khi halt để kết luận jitter lúc đang chạy.

Giữ log gốc. Không kết luận từ vài dòng cuối; tìm sự kiện đầu tiên trước fault/stale.

## 5. Thứ tự cải tiến đề nghị, chưa triển khai

### P0 — đường lỗi không làm mất chẩn đoán

Giữ nguyên gain, observer, bảng motor, cơ khí và giới hạn R12. Bổ sung nhật ký nhỏ, có giới hạn và không ghi Flash trong ISR: lỗi I2C đầu tiên, mã HAL, SR1/SR2, mức SDA/SCL, thời gian đọc thực, tuổi mẫu, trạng thái FSM, UART/TX busy/drop và thời điểm phục hồi. Gửi bằng gói chẩn đoán riêng để giữ nguyên frame balance 122 byte và parser hiện hành.

Khi mất IMU: dừng drive theo chính sách đã kiểm chứng; không tiếp tục vòng retry blocking 200 Hz trong EMERGENCY. Lỗi phải còn đọc được qua đường trạng thái độc lập với việc có frame cảm biến mới.

### P1 — phục hồi bus và sensor có giới hạn

FSM ngoài ngắt điều khiển: ghi lỗi → giữ motor an toàn → chờ/backoff → kiểm mức bus → bus clear khi thích hợp → reset peripheral → probe chip → reset/configure sensor nếu cần → kiểm return từng lệnh và readback/PMU → xác nhận mẫu mới, cấu hình, tư thế và độ yên → chờ người dùng ARM mới.

Chỉ có một chủ sở hữu giao dịch I2C, không cho TIM4 và luồng phục hồi truy cập đồng thời. Dùng deadline thực cho toàn đường lỗi; kiểm cả nhánh BUSY trong HAL, không chỉ tham số 2 ms. Nếu lỗi còn tồn tại, chuyển sang trạng thái lỗi cố định nhưng còn giao tiếp; không reset lặp vô hạn.

Không tự hiệu chuẩn khi xe đang rung/ngã. Không tự ARM và không phát lại nút giữ/epoch cũ sau sensor/MCU phục hồi. Nếu SCL bị giữ LOW hoặc sensor không nhả bus, reset MCU không bảo đảm reset sensor; giải pháp cấp lại nguồn IMU có điều khiển là tùy chọn phần cứng cần thiết kế riêng. Phải tránh cấp ngược qua SDA/SCL/CS/INT; chưa tự thêm vào PCB.

### P2 — pipeline đo lường sau khi đã có số đo và test lỗi

Có thể chuyển sang lấy mẫu bất đồng bộ (I2C IT/DMA), timestamp và buffer hoàn chỉnh; timer điều khiển chỉ đọc snapshot nguyên tử đủ mới. Cần deadline, phát hiện sample stale/config reset, abort/error callbacks và recovery. INT1 bị mất/bị giữ mức phải có đường phát hiện; pipeline không được đợi ngắt mãi.

Giữ ODR/filter/dt nền trước; không tăng IMU từ 100 lên 200 Hz hoặc đổi lọc đồng thời với thay kiến trúc vì sẽ khó truy nguyên hồi quy. Phân biệt tuổi dữ liệu sensor với thời điểm DMA hoàn tất; không dùng sensortime đang chạy ở suspend để nhận nhầm sensor đã có dữ liệu mới.

### P3 — watchdog và phần cứng

Bổ sung giám sát sống riêng của STM32: thời điểm vòng điều khiển và foreground phục vụ, reset flags khi boot, fault context có cơ chế lưu phù hợp. Watchdog chỉ được nuôi khi các luồng quan trọng đã tiến triển; trong trạng thái lỗi IMU còn điều khiển được, supervisor/telemetry vẫn phải hoạt động để tránh reset storm.

Trước khi bật watchdog phải thử trạng thái chân PWM trong reset/boot, phanh/coast theo tải và nguồn, yêu cầu ARM mới, và phát hiện lỗi sensor dai dẳng. Không kỳ vọng watchdog sửa dây lỏng hoặc hỏng nguồn.

PCB: đầu nối có giữ cơ khí, IMU cố định đúng hướng, điểm đo 5V/3V3/VM/GND/SCL/SDA/UART, tụ sát module, đường hồi công suất phù hợp. Kiểm pull-up thực tế/rise time trước khi thêm điện trở; không thêm tụ lọc lớn vào SCL/SDA. Không sửa sơ đồ C1–C9 hoặc cơ khí từ suy đoán khi chưa đo.

## 6. Ma trận kiểm thử đề nghị

Thử từ ít rủi ro tới nhiều rủi ro, mỗi bài có baseline so sánh RT8. Các con số lượt thử dưới đây là mục tiêu ban đầu của project, không phải tiêu chuẩn chứng nhận. Tăng mức chỉ sau khi mức trước đạt; ngừng khi có reset không giải thích được, nóng bất thường hoặc mất quyền STOP.

| Nhóm | Tình huống | Cách thử / phạm vi | Điều kiện đạt |
|---|---|---|---|
| A — hồi quy | Đứng yên, dựng, tiến/lùi, đảo chiều, quay và 4 chéo | Giữ điều kiện/VM đã chốt; 10 lượt mỗi nhóm, trước ở tốc độ thường | Không mất chức năng R12 đã tốt; nhả hãm đúng; không có lệnh cũ chạy lại |
| B — IMU lỗi ngắn | NACK, một hoặc vài giao dịch thất bại | Fault injection trong mock/HIL trước, motor ngắt khỏi nguồn động lực khi kiểm bus vật lý | Drive về trạng thái an toàn, lỗi được ghi, foreground còn sống; không che lỗi bằng mẫu cũ |
| C — bus kẹt | SDA LOW; SCL LOW; bus trở lại bình thường | Fixture open-drain 3,3 V có kiểm soát, xe cố định/motor vô hiệu; không dùng dây chập nguồn hay hot-unplug khi đang cân bằng | Retry có giới hạn; SDA/SCL phân biệt đúng; lỗi kéo dài vẫn chẩn đoán được; nhả fixture có thể về trạng thái chờ |
| D — sensor reset/config | BMI160 reset, sai ODR/range, mẫu cũ hoặc data-ready mất | Mô phỏng driver/HIL, rồi mới fixture thật khi drive đã vô hiệu | Phát hiện lỗi cấu hình/tuổi mẫu; khởi tạo lại đúng; không tự ARM |
| E — communication | Byte UART lỗi, mất khung, CRC sai, TX busy, ngắt dây UART | Inject/mock trước; thử vật lý khi motor đã an toàn | Không nhận lệnh lỗi/replay; thống kê lỗi tăng; nối lại không phát lại MOVE cũ |
| F — web/network | Ẩn tab, khóa màn hình, mất WS, reconnect | Dùng chức năng đang có, không đổi watchdog/lease | Lease hết thì hãm/giữ cân bằng đúng thiết kế; STOP và session mới đúng; gateway không reset vô cớ |
| G — nguồn | Điện áp pin trong vùng vận hành đã dùng; motor khởi động/phanh/đảo chiều | Đo tại tải, không cố xả pin dưới mức người dùng đang áp dụng, không gây ngắn mạch | Không có sụt/vọt áp vượt giới hạn linh kiện; không reset không rõ nguyên nhân |
| H — cơ khí | Gồ nhẹ, mối nối sàn, bánh bám không đều, chạm thân có kiểm soát | Ghi kích thước/mức kích thích; giữ khối lượng/CG hiện hành; 10 lượt có kiểm soát | Trong miền đã chọn: cân bằng/giảm tốc; ngoài miền: dừng an toàn, không mất chẩn đoán |
| I — ngã có kiểm soát | Ngã trước/sau và lệch bên khi đã chuẩn bị đỡ | Mặt đệm/giá đỡ phù hợp, bắt đầu mức thấp, không chủ ý đập cảm biến xuống nền cứng | Sau lỗi có thể phục hồi chẩn đoán; không cần tháo pin với lỗi có thể khôi phục; không tự chạy lại |
| J — nhiệt/thời gian | Chu kỳ giữ/quay/đổi chiều lâu hơn bình thường | Khởi đầu 15–30 phút có giám sát, chia các lượt log; đo nhiệt motor/driver/buck, không thử stall kéo dài | Không tăng lỗi theo nhiệt; không tràn hoặc mất điều khiển khi log đầy |
| K — lưu trữ/tải web | RAM log đầy, archive đầy, tải dữ liệu sau STOP | Test các giới hạn có sẵn, không xóa log gốc chưa lưu | Đầy log không làm dừng xe hoặc mất STOP; báo lỗi rõ; không tự ghi đè |
| L — sự cố MCU | HardFault giả lập, foreground không tiến triển, ISR kéo dài | Trên fixture motor vô hiệu trước; dùng pin đo/logic để kiểm đầu ra | Watchdog/đường dừng có thời gian xác định; boot an toàn, không tự ARM/reset lặp |

### Chỉ số bắt buộc ghi

- Số lần reset STM32 và ESP32, reset reason, số lần cần tháo pin.
- Số lần lỗi I2C theo loại; SDA/SCL, thời gian giao dịch cực đại và tuổi mẫu mới thực tế.
- dt/execution vòng điều khiển: phân vị 50/95/99, max, số lần ngoài 4–6 ms; đo thêm đường lỗi vì frame telemetry có thể không ghi được lúc main đói.
- Khoảng cách lớn nhất giữa hai frame hợp lệ; CRC errors, sequence gaps, queue drops, TX busy duration.
- Pitch/gyro, v_ref/v_actual, x_ref−x, yaw/w_ref, REC/cancel/fault, PWM từng bánh và thời gian bão hòa.
- Điện áp nhỏ nhất/lớn nhất đo bằng thiết bị, nhiệt đo được; không thay bằng số VM giả định trong mô phỏng.

RAM balance 9000 frame xấp xỉ 180 s. Để giữ log đầy đủ, dùng các lượt 60–120 s, STOP, chờ 2 s rồi tải/lưu trước khi xóa. Thử endurance liên tục dài hơn cần bộ ghi ngoài hoặc cơ chế ghi tình trạng thưa riêng; archive hiện có không được mặc định là ghi liên tục vô hạn. Ghi Flash là best-effort, có thể mất đuôi khi mất nguồn.

### Tiêu chí mục tiêu cho bản cải tiến

1. Không chủ ý dùng dữ liệu IMU không hợp lệ để tiếp tục cân bằng. Lỗi nằm ngoài miền phục hồi thì dừng an toàn.
2. Với nguồn còn hợp lệ, lỗi cảm biến không làm mất kênh chẩn đoán vô thời hạn. Mục tiêu ban đầu có thể đặt status 5–10 Hz độc lập trong FAULT; chưa phải khả năng RT8 hiện có được bảo đảm.
3. Khi fixture lỗi được tháo, bus/sensor phục hồi có giới hạn và về trạng thái chờ, không tự ARM; thời gian mục tiêu phải chốt sau đo các bước reset/khởi tạo/hiệu chuẩn.
4. Không thay ngưỡng va đập/góc để biến lượt FAIL thành PASS. Đánh giá riêng khả năng cứu xe và khả năng sống sót phần điện tử sau ngã.
5. Chuỗi test không có lỗi chưa chứng minh xác suất hỏng bằng 0; ghi cả số lượt, mức kích thích và điều kiện thử.

## 7. Kết luận và việc nên làm tiếp

Ưu tiên trước là **khoanh đường lỗi I2C có thể làm đói foreground, duy trì chẩn đoán sau fault và bổ sung phục hồi bus/sensor có giới hạn**. Đo nguồn và chất lượng tín hiệu song song để xử lý tác nhân khởi phát. Chưa có bằng chứng cần thay luật R12 hoặc nới các bảo vệ.

Nếu triển khai tiếp, tách thành các thay đổi nhỏ: (1) chẩn đoán + test lỗi thực của driver, (2) supervisor/recovery khi motor đã an toàn, (3) pipeline nếu số đo cho thấy cần, (4) watchdog/PCB và chiến dịch thử có kiểm soát. So sánh A/B cùng tình huống, giữ bản RT8 hoạt động tốt làm mốc; không coi nhánh PipeLine đã có là đã được review hay tương thích mặc định.
