# Kiểm tra firmware trên xe breadboard và dự trữ motor ở 72 cm/s

Ngày 31/10/2026. Source STM32 R12 RT8 / ESP32 RT8 NET5 tại commit `b95cd71`.

Phần cứng đánh giá là **xe breadboard hiện tại**. Không giả định đã lắp PCB, tụ C1–C9 hay thay cách phân phối nguồn. Người dùng xác nhận xe vận hành tốt bình thường; ngã mạnh đôi khi làm lỗi BMI160/mất dữ liệu STM32, Wi-Fi còn nhưng phải rút pin để khôi phục.

Lượt này đọc mã, chạy hồi quy và chèn lỗi trong mô hình phần cứng trên máy. **Chưa thay firmware, tốc độ, gain, observer, hiệu chuẩn hoặc cơ khí; chưa nạp xe và chưa đo nguồn/dòng trên xe.**

## 1. Kiểm tra đã thực hiện

### Hồi quy hiện hành

Đã chạy thành công 9 CJS: controller, dashboard, gateway, session, recovery, net1 routes, RT8 tracking, RT8 replay, RT8 A/B. Kiểm tra giữ–thả, ARM/STOP, epoch/sequence, reconnect không phát lại lệnh, guard phục hồi, tracking và log vẫn qua trên host. Hai header R12 giữa firmware/mô phỏng và header giao thức giữa hai bo được kiểm tra khớp.

Replay sử dụng dữ liệu log27 **RT7**, 7.585 mẫu: legacy=3 lần REC, RT8=0. Đây là replay dữ liệu cố định; không phải log mới ở 72 cm/s của firmware RT8 đang nạp trên xe.

### Chèn lỗi vào mã thực trên host

| Kiểm tra | Kết quả quan sát | Giới hạn của bằng chứng |
|---|---|---|
| Trích đúng helper chờ flag từ HAL I2C đang dùng, giữ BUSY và cho SysTick giả lập tăng | Đường BUSY dùng timeout 25 ms; harness ghi 27 ms theo cách tăng tick giả lập, dù tham số đọc ở driver là 2 ms | 27 ms không phải đo trên STM32. Bằng chứng xác nhận đường chờ vượt chu kỳ timer 5 ms có tồn tại |
| Biên dịch driver `bmi160.c` thực với CHIP_ID đúng nhưng cả 7 lần ghi I2C thất bại | `BMI160_Init()` vẫn trả `true` | Robot_Init còn readback config riêng, nên không được kết luận toàn bộ FSM luôn chấp nhận sensor lỗi. Tuy nhiên lỗi ghi từng bước/mode chưa được xử lý trực tiếp |
| Biên dịch FSM thực, ARM rồi ép đọc IMU thất bại trong 201 vòng | Fault 32, state EMERGENCY, hai lệnh motor bằng 0; 201 lần đọc vẫn được gọi | Mocks motor xác nhận lệnh phần mềm, không đo đầu ra A4950 hoặc mô phỏng preemption thật |
| Sau đó cho đọc IMU trở lại bình thường thêm 45 vòng | FSM vẫn EMERGENCY, chưa ready, không tự ARM | Việc không tự chạy lại là bảo vệ đúng; thiếu phục hồi tự động có kiểm soát không đồng nghĩa nên tự xóa fault rồi chạy |

Các harness và kết quả nằm trong `.build/runtime_motor_audit/`; không thay mã sản phẩm. Harness vận chuyển I2C, nguồn, clock và plant đều là giả lập; không chứng minh đã tái hiện chính xác cú ngã của xe.

## 2. Kết luận về độ mượt/ổn định hiện tại

**Chưa đạt căn cứ để xác nhận ổn định xuyên suốt mọi tình huống.** Mã điều khiển bình thường qua hồi quy, nhưng đường lỗi còn các khoảng trống:

1. Đọc I2C blocking trong TIM4 có nhánh chờ BUSY 25 ms. Lặp lỗi trong ngắt có thể làm main thiếu thời gian xử lý UART/telemetry; cần đo để xác nhận trên xe.
2. Sau fault IMU, việc thử đọc vẫn chạy mỗi vòng, chưa có backoff/recovery bus trong foreground.
3. Đo lại gyro hiện không giải phóng bus hoặc gọi lại toàn bộ BMI160_Init. Sensor bị reset/suspend hoặc giữ bus có thể không phục hồi bằng thao tác này.
4. HardFault handler còn vòng lặp vô hạn; IWDG chưa được bật. Trạng thái điện PWM khi CPU treo phải kiểm tra, không suy từ số 0 trên giao diện.
5. Chưa có đo ADC dòng motor hoặc VM trực tiếp; chưa có trường tuổi mẫu IMU riêng, diagnostic HAL chi tiết hoặc reset reason STM32 đủ để xác định sự kiện đầu tiên.

Ưu tiên phần mềm trước khi nói tới tăng tốc: giữ motor an toàn, giữ kênh diagnostic sống sau fault, chặn đường chờ I2C vượt ngân sách, kiểm lỗi ghi config và phục hồi bus/sensor có giới hạn ngoài ISR. Không tự ARM hoặc phát lại lệnh cũ sau phục hồi. Các thay đổi này chưa được triển khai trong lượt kiểm tra.

UART 115200/8N1 có năng lực lý thuyết 11.520 byte/s. Frame balance 122 byte × 50 Hz = 6.100 byte/s, khoảng 53% năng lực một chiều TX trước metadata; mỗi frame mất khoảng 10,59 ms. Không thấy căn cứ chỉ từ mức dữ liệu danh nghĩa để kết luận baud thiếu. Cần ưu tiên phát hiện CPU không phục vụ queue, TX DMA bận dai dẳng, lỗi UART và CRC. Hai chiều UART là độc lập, không cộng RX lệnh vào cùng ngân sách TX này.

### Điều kiện cần đo trên breadboard

- Đo 3,3 V ngay tại BMI160/STM32 và 5 V tại ESP32 khi tăng tốc, phanh, đảo chiều; dùng oscilloscope để thấy xung. Điện áp tĩnh tại buck chưa đủ.
- Kiểm chân cắm, dây và giữ cơ khí khi xe ngã; software không thể đọc một sensor bị mất nguồn hoặc đứt tiếp xúc liên tục.
- Kiểm pull-up đã có trên GY-BMI160 và dạng sóng PB8/PB9. Không tự thêm điện trở song song khi chưa biết giá trị tương đương.
- Khi lỗi tái hiện và xe đã an toàn: lưu JSON network, boot_id/uptime, dữ liệu cuối cùng nếu tải được; quan sát SCL/SDA và TX PB6. Wi-Fi còn không chứng minh nguồn STM32/BMI160 bình thường.
- Thử chuyển động 60–120 s/lượt, tải log sau STOP và chờ 2 s. Log RAM 9000 frame không đủ ghi liên tục vô hạn; bài nóng máy dài phải chia lượt hoặc dùng bộ ghi ngoài.

Các bước đo và FSM cải tiến chi tiết: [Đánh giá độ bền RT8/BMI160](Danh_Gia_Do_Ben_RT8_BMI160_20261031.md).

## 3. 72 cm/s đang sử dụng bao nhiêu khả năng của motor?

### 3.1. Tốc độ bánh

Nguồn: `thông số động cơ.txt`, `HANDOFF.md`, `balance_motion.h` và `phase1_catalog_params.h`. Dùng bánh đường kính **65 mm** đã chốt, không dùng lại bán kính 35 mm.

```text
Chu vi bánh = π × 0,065 = 0,2042035 m
RPM ở 0,72 m/s = 0,72 / 0,2042035 × 60 = 211,5536 vòng/phút
```

| So sánh | Giá trị |
|---|---:|
| Tốc độ bánh ở 72 cm/s | 211,55 rpm |
| So với 280 rpm không tải, theo file thông số | 75,55% |
| So với 215 rpm có tải, theo file thông số | 98,40% |
| Tốc độ tuyến tính tương ứng 280 rpm | 95,29 cm/s |
| Tốc độ tuyến tính tương ứng 215 rpm | 73,17 cm/s |

215 rpm là thông tin nhà bán trong file, không phải trần tốc độ đo trên xe này. Vận tốc trên tính khi bánh lăn không trượt; encoder không tự chứng minh tốc độ tịnh tiến trên mặt sàn trơn. 72 cm/s là reference preset nhanh, không bảo đảm xe đạt đúng mức đó trong mọi lượt.

**75,55% tốc độ không tải không đồng nghĩa motor mới dùng 75,55% công suất hoặc còn 24,45% lực để cứu xe.** Tốc độ, PWM, dòng, mô-men, nhiệt và công suất là các đại lượng khác nhau. Quan hệ tốc độ–mô-men của motor DC và dòng thay đổi theo tải: [maxon, Motor Data and Operating Ranges](https://www.maxongroup.com.tw/medias/sys_master/8798985748510.pdf?attachment=true), [Pololu, Motors and Gearboxes](https://www.pololu.com/docs/0J21/5.c).

### 3.2. Tính trực tiếp bằng bộ phân bổ firmware

Đã biên dịch và gọi **hàm `cat_allocate()` thực**, dùng `CAT_PARAM`, `CAT_VNL`, `CAT_FRICTION` và `CAT_START` thực; không fit lại motor hoặc thay tham số để có kết quả mong muốn. Giả sử hai bánh cùng chạy 0,72 m/s, yaw request=0. `total` dưới đây là lực thuần cặp bánh yêu cầu sau mô hình bù; chưa phải lực đã đo trên xe.

| Hướng | Tổng lực yêu cầu | PWM trái / phải | % biên độ PWM trái / phải |
|---|---:|---:|---:|
| Tiến | 0 N | 1943 / 1982 | 77,75% / 79,31% |
| Tiến | +1 N | 2205 / 2244 | 88,24% / 89,80% |
| Tiến | +2 N | 2472 / 2499 | 98,92% / 100% |
| Lùi | 0 N | −2011 / −1908 | 80,47% / 76,35% |
| Lùi | −1 N | −2273 / −2169 | 90,96% / 86,79% |
| Lùi | −2 N | −2499 / −2466 | 100% / 98,68% |

Tỷ lệ là `abs(PWM)/2499`. Motor driver dùng slow decay nên không đồng nhất số này với thời gian một chân điều khiển đơn lẻ ở HIGH. Đây là đầu ra tính từ model; **không phải PWM trung vị hay công suất đo của xe đang chạy 72 cm/s**. Trường hợp lực 0 N vẫn có PWM vì mô hình bù sức điện động phản kháng và ma sát; không phải khẳng định chạy trên sàn thực không cần lực.

Các tham số là mô hình quanh VM tham chiếu lịch sử 11,7 V, không bù theo điện áp pin đo trực tiếp. `Simulation/current/motor.sci` lấy một phần từ datasheet/suy tính; lực dưới tải chưa được nhận dạng độc lập. Không coi việc gọi hàm thật biến mô hình motor thành phép đo vật lý.

### 3.3. Dự trữ lực trong chính mô hình hiện tại

Theo giới hạn từng bánh của allocator, tổng giới hạn lực thuần cùng chiều tại các tốc độ xấp xỉ:

| Tốc độ thẳng | Tiến | Lùi, trị tuyệt đối |
|---|---:|---:|
| 50 cm/s | 4,255 N | 4,264 N |
| 72 cm/s | 2,051 N | 2,064 N |
| 80 cm/s | 1,250 N | 1,263 N |

Đây là tổng giới hạn mô hình ở thời điểm xét, **chưa trừ lực đã dùng cho tải/gia tốc** và chưa xét yaw phải chia lực giữa hai bánh. Không thể gọi toàn bộ 2,05 N là lực dự phòng còn lại trong lượt chạy thật. Nhưng phép so sánh giải thích vì sao tăng tốc làm giảm khả năng bù thêm lực cùng chiều: từ 50 lên 72 cm/s, giới hạn này trong model giảm khoảng một nửa.

Không được suy ra lực có thể dùng ở chiều phanh bằng cùng bảng này; giới hạn ngược chiều và trạng thái driver/motor khác. Tương tự, 0,8 m/s là guard tốc độ cân bằng thường, không phải reference cho phép chạy liên tục.

### 3.4. Chạy chéo dùng bánh ngoài nhanh hơn tốc độ tâm xe

Với track mô hình 0,262 m, tâm xe 0,72 m/s và yaw rate 0,55 rad/s (preset nhanh trước governor):

```text
v_bánh trong = 0,72 − 0,262 × 0,55 / 2 = 0,64795 m/s
v_bánh ngoài = 0,72 + 0,262 × 0,55 / 2 = 0,79205 m/s
RPM bánh ngoài ≈ 232,72 rpm
```

Track còn là số suy từ hình học. Các con số trên là kinematics lý tưởng, chưa tính governor, slip, góc thân và yêu cầu mô-men quay. Bánh ngoài có thể cần gần 79,2 cm/s ngay khi tâm chỉ chạy 72 cm/s; vì vậy không đánh giá dự trữ motor chỉ từ tốc độ trung bình hai bánh.

## 4. Chưa thể xác định phần trăm công suất thực

Xe chưa đo dòng motor/điện áp motor trực tiếp, mô-men tải và nhiệt trong đúng lượt 72 cm/s. Do đó chưa đủ số liệu để đưa ra phần trăm công suất điện, công suất cơ, tải định mức hay tuổi thọ.

`P_cơ = mô-men × tốc độ góc`; `P_điện` cần điện áp/dòng phù hợp với cách PWM và thời điểm đo. Mô-men 0,4 kgf·cm và mức tối đa 2 kgf·cm trong file không được dùng thay cho mô-men thực đang sinh ra. Không coi mô-men tối đa là mô-men cho phép liên tục. Không ngoại suy thẳng đường đo không tải ở PWM 200–600 thành dữ liệu công suất ở PWM khoảng 2000 dưới tải.

Log cần cho đánh giá thực tế: giữ tiến và lùi mức nhanh trên nền đã xác định, lưu riêng các đoạn đủ ổn định; tính median/p95/max `abs(pwm_l/r)/2499`, thời gian gần bão hòa, vận tốc từng bánh, reference achieved, pitch/gyro, REC/cancel/fault. Đo VM tại driver dưới tải, dòng và nhiệt nếu cần đánh giá công suất/dự trữ nhiệt. Chưa có log RT8 mới như vậy trong lượt này.

## 5. Kết luận sử dụng

- Giữ trần reference hiện hành 72 cm/s, không mở thêm tốc độ từ tỷ lệ 75,55% RPM.
- Ở mốc này xe đã gần mức tốc độ có tải 215 rpm ghi trong file; model allocator cho thấy PWM nền khoảng 76–81% trước khi cộng lực thuần/yaw và có thể lên 100% khi cần bù thêm lực.
- Ưu tiên giải quyết đường lỗi IMU/foreground để ngã không làm mất giao tiếp kéo dài; không hy vọng đổi PCB hoặc tụ chưa lắp sẽ tự giải quyết cho breadboard hiện tại.
- Muốn xác nhận độ mượt xuyên suốt cần đo timing/nguồn và thử lỗi thực có kiểm soát. Các test host vừa chạy xác nhận hành vi mã và chỉ ra điểm yếu, không chứng nhận hệ cơ điện ngoài thực tế.
