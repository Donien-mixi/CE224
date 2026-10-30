# HƯỚNG DẪN ĐI DÂY HỆ THỐNG XE HAI BÁNH TỰ CÂN BẰNG

Cập nhật: **30/10/2026**. Project: `D:\DA_HTN`.

Tài liệu ghi lại cấu hình kết nối của đồ án đang sử dụng, dựa trên sơ đồ đi dây đã lưu, bố trí phần cứng đã được người dùng xác nhận và đối chiếu mã nguồn hiện hành. Xe đã được người dùng đánh giá hoạt động cân bằng ổn định. Việc cập nhật tài liệu không yêu cầu thay dây hoặc hiệu chuẩn lại xe đang chạy tốt.

Firmware nguồn dùng để đối chiếu:

- STM32: `BAL-20261024-R12-8DIR-RT8`.
- ESP32: `TWIP-WEB-20261024-BAL-R12-8DIR-RT8-NET5`.

Đây là phiên bản source trên laptop; không dùng thông tin này để khẳng định hai bo trên xe đã được nạp RT8 hoặc ba thay đổi RT8 đã được thử thực tế. Xem thêm [HANDOFF.md](HANDOFF.md).

## 1. Quy ước hướng và bố trí thực tế

Đứng **phía sau xe nhìn về phía trước** để xác định bên trái và bên phải:

| Vị trí | Cụm linh kiện / quy ước |
|---|---|
| Phía trước (+x) | WAGO 1 phân phối nguồn pin LiPo |
| Phía sau (−x) | WAGO 2 phân phối nguồn 5 V sau XL4015 |
| Bên trái | Motor trái, encoder TIM2, kênh A của driver |
| Bên phải | Motor phải, encoder TIM3, kênh B của driver |
| Tầng 1, trên mica thứ nhất | Pin LiPo 3S và XL4015 |
| Dưới mica thứ hai | Hai cụm WAGO; nguồn pin phía trước, nguồn 5 V phía sau |
| Tầng 2, trên mica thứ hai | Breadboard chứa STM32, ESP32-S3, BMI160 và Dual A4950 |
| Cọc giữa các tấm mica | 55 mm |

**Driver A4950 nằm trên breadboard tầng 2.** Phải phân biệt vị trí đặt module với đường đi dòng điện: VM và các đầu ra motor nối bằng dây riêng theo bảng bên dưới, không dùng ray nguồn breadboard để phân phối dòng motor.

Giữ nguyên hướng lắp BMI160 đã dùng khi hiệu chuẩn xe. Chỉ mô tả “mặt chip ngửa lên” không đủ xác định hướng các trục X/Y/Z; không xoay cảm biến hoặc thay lớp đệm lắp đặt chỉ để khớp một hình minh họa.

## 2. Sơ đồ nguồn tổng thể

**Xác nhận bổ sung của người dùng:** breadboard hiện tại không lắp tụ ngoài; INT1 của BMI160 đã nối PB2; còi hai chân có (+) → PB12, (−) → GND. Theo yêu cầu mới, sơ đồ KiCad revision 1.2 bổ sung **C1–C9 để lắp trên PCB mới**, không đánh dấu DNP. Các tụ này là phần thiết kế bổ sung, không phải linh kiện đã có trên breadboard.

```text
Pin LiPo 3S (+) ── WAGO 1 (+) ──┬── VM của Dual A4950
                                └── IN+ của XL4015
Pin LiPo 3S (−) ── WAGO 1 (−) ──┬── GND hàng 1 của Dual A4950
                                └── IN− của XL4015

XL4015 OUT+ (5 V) ── WAGO 2 (+) ──┬── Ray 5 V breadboard
                                  ├── VCC của Dual A4950
                                  └── (+) tụ 1000 µF / 16 V [PCB mới sẽ lắp; breadboard hiện chưa có]
XL4015 OUT− ──────── WAGO 2 (−) ──┬── Ray GND breadboard
                                  ├── GND hàng 2 của Dual A4950, dây riêng
                                  └── (−) tụ 1000 µF / 16 V [PCB mới sẽ lắp; breadboard hiện chưa có]

Ray 5 V ──┬── 5V của STM32 Black Pill
           └── 5V/VIN của bo ESP32-S3 đang dùng

STM32 3V3 ──┬── BMI160 3V3
             ├── BMI160 CS
             ├── VCC encoder trái
             └── VCC encoder phải
```

“12 V” là cách gọi nhánh nguồn pin 3S, không phải đầu ra ổn áp 12 V. Driver nhận VM trực tiếp từ pin. Điện áp khoảng 11,7 V là số đo lịch sử của người dùng, không phải phép đo mới hoặc số đo ADC từ firmware.

### 2.1. WAGO 1 — nguồn pin

| Điểm nối | Dây đến |
|---|---|
| WAGO 1 (+) | Cực dương pin qua XT60, driver VM, XL4015 IN+ |
| WAGO 1 (−) | Cực âm pin qua XT60, driver GND hàng 1, XL4015 IN− |

### 2.2. WAGO 2 — nguồn 5 V

| Điểm nối | Dây đến |
|---|---|
| WAGO 2 (+) | XL4015 OUT+, ray 5 V breadboard, driver VCC, cực dương C1 trên PCB mới (breadboard chưa có) |
| WAGO 2 (−) | XL4015 OUT−, ray GND breadboard, driver GND hàng 2 qua dây riêng, cực âm C1 trên PCB mới (breadboard chưa có) |

Giữ nhánh VCC driver lấy từ WAGO 2 (+) như sơ đồ đang lưu. Chưa có sơ đồ mạch module và giá trị điện trở cảm dòng để kết luận cấp 5 V tại chân này tương ứng giới hạn dòng 2 A. Không điều chỉnh nguồn hoặc thay linh kiện cảm dòng từ suy luận đó.

### 2.3. Ray nguồn trên breadboard

| Ray / điểm phân phối | Các chân nối vào |
|---|---|
| 5 V | STM32 5V, ESP32-S3 5V/VIN |
| GND | GND STM32, ESP32-S3, BMI160, encoder trái/phải; BMI160 SA0; chân âm còi |
| 3,3 V từ STM32 | BMI160 3V3 và CS; VCC encoder trái/phải |

Các nhánh cùng dùng mốc GND hệ thống. Việc đi dây GND riêng nhằm tránh dùng chung đoạn dây/ray mang dòng motor với tín hiệu; **không tạo cách ly điện giữa hai khối**. Không có phép đo xác nhận các số spike dòng, ground bounce hoặc sai số gyro cụ thể trong tài liệu cũ, nên không dùng chúng làm kết luận về xe.

### 2.4. Tụ lọc bổ sung trên PCB mới — schematic revision 1.2

Các tụ C1–C9 đều được vẽ thành kết nối điện và đánh dấu **sẽ lắp**, với chân 1 nối rail nguồn tương ứng, chân 2 nối GND. Chúng mắc song song nguồn, không mắc nối tiếp đường cấp nguồn. Các tụ đã có sẵn bên trong từng module vẫn giữ nguyên.

| Ký hiệu | Giá trị / điện áp định mức tối thiểu | Nối giữa | Vị trí đặt khi thiết kế PCB |
|---|---|---|---|
| C1 | 1000 µF / 16 V, điện phân | +5V và GND | Gần OUT+/OUT− XL4015 và điểm phân phối 5 V |
| C2 | 470 µF / 35 V, điện phân | VBAT_3S và GND | Gần đầu VM/GND của module Dual A4950 |
| C3 | 220 nF / 50 V, gốm X7R | VBAT_3S và GND | Sát đầu VM/GND module driver, vòng dòng ngắn |
| C4 | 10 µF / 16 V, gốm X7R | +5V và GND | Sát chân cấp nguồn 5V/GND của bo ESP32-S3 |
| C5 | 100 nF / 16 V, gốm X7R | +5V và GND | Cùng vị trí C4, ưu tiên đường nối ngắn |
| C6 | 10 µF / 16 V, gốm X7R | +5V và GND | Sát chân cấp nguồn 5V/GND Black Pill |
| C7 | 100 nF / 16 V, gốm X7R | +5V và GND | Cùng vị trí C6, ưu tiên đường nối ngắn |
| C8 | 1 µF / 16 V, gốm X7R | +3V3 và GND | Sát chân 3V3/GND của module BMI160 |
| C9 | 100 nF / 16 V, gốm X7R | +3V3 và GND | Cùng vị trí C8, ưu tiên đường nối ngắn |

C1/C2 chọn loại điện phân 105 °C có thông số ESR và dòng ripple phù hợp; chân dương hướng về rail nguồn, chân âm về GND. Chọn kích thước tụ gốm theo điện dung hiệu dụng tại điện áp làm việc (DC bias), không chỉ trị số in trên linh kiện. Footprint và mã linh kiện chưa được chốt.

Đây là **phương án tụ ngoài cho bo mang các module hiện có**, chưa được đo trên xe. Không coi các trị số trên là đã được tối ưu thực nghiệm. Khi thử bo cần kiểm tra XL4015 khởi động với C1 1000 µF, đáp ứng tải 5 V, nguồn ESP32 khi phát Wi-Fi và VM lúc motor khởi động/phanh/đảo chiều. Tụ không thay thế nguồn đủ dòng, đường đồng phù hợp và đường hồi GND được bố trí đúng. Không cho dòng motor dùng chung đoạn đường hồi GND với BMI160; các khối vẫn chung mốc GND.

Cơ sở thiết kế: [Allegro A4950, Application Information](https://www.allegromicro.com/-/media/files/datasheets/a4950-datasheet.pdf) khuyến nghị tụ điện phân song song tụ gốm gần nguồn tải; [Espressif ESP32-S3, Power Supply](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html#power-supply) hướng dẫn tụ đệm tại đầu nguồn và tụ decoupling. Giá trị C2 470 µF và bộ tụ ngoài module ở đây là lựa chọn thiết kế cho project, không phải giá trị bắt buộc của hãng. Các khuyến nghị tại chân chip phải phân biệt với tụ bổ sung ở đầu vào 5 V của bo ESP32-S3 có sẵn LDO và tụ nội bộ.

## 3. STM32 và ESP32-S3

| Chân STM32 | Chân ESP32-S3 | Chức năng |
|---|---|---|
| PB6 — USART1_TX | GPIO18 — Serial1 RX | STM32 gửi dữ liệu sang gateway |
| PB7 — USART1_RX | GPIO17 — Serial1 TX | Gateway gửi lệnh sang STM32 |
| GND hệ thống | GND hệ thống | Mốc tín hiệu chung |

UART **115200 baud, 8N1**, tín hiệu logic 3,3 V. Nguồn 5 V đi vào chân cấp nguồn của từng bo, không đưa vào chân UART.

Dữ liệu balance hiện được gửi **50 Hz** từ vòng điều khiển **200 Hz**. Tần số gửi dữ liệu khác tần số lấy mẫu cảm biến và tần số cập nhật giao diện web. STM32 nhận lệnh ARM, STOP, điều khiển 8 hướng và các lệnh đo/hiệu chuẩn; luật cân bằng hiện tại là R12 với observer và phân bổ lực từng bánh.

## 4. BMI160 và STM32

Giữ cách cấp nguồn trực tiếp 3,3 V của module đang dùng; bỏ trống VIN như sơ đồ đi dây đã lưu.

| Chân GY-BMI160 | Điểm nối | Vai trò hiện tại |
|---|---|---|
| 3V3 | STM32 3V3 | Nguồn cảm biến |
| GND | Ray GND | Mass chung |
| SCL | STM32 PB8 | I2C1 SCL, 400 kHz |
| SDA | STM32 PB9 | I2C1 SDA |
| CS | STM32 3V3 | Giữ mức HIGH để dùng I2C |
| SA0 / SDO | Ray GND | Chọn địa chỉ I2C 0x68 |
| VIN | Bỏ trống | Không dùng trong cách cấp nguồn này |
| INT1 | STM32 PB2 — người dùng xác nhận đã cắm | Dây đã nối; firmware hiện chưa dùng ngắt này để chạy cân bằng |
| INT2 | Bỏ trống | Không sử dụng |

**Trạng thái INT1:** người dùng xác nhận dây **INT1 → PB2 đang cắm trên breadboard**; giữ kết nối này trên sơ đồ và PCB mới. Mã GPIO vẫn khai báo EXTI2, nhưng firmware hiện không cấu hình phát data-ready qua INT1 và không dùng callback này để đọc IMU/chạy cân bằng. Phân biệt dây đã nối với việc firmware có sử dụng ngắt đó hay không.

Cảm biến được cấu hình accel ±8 g, gyro ±2000°/s, ODR **100 Hz**. STM32 đọc I2C bằng HAL blocking, timeout 2 ms, trong vòng TIM4 **200 Hz**. Không mô tả đây là pipeline dùng INT1 hoặc I2C DMA.

Firmware thử địa chỉ 0x68 rồi 0x69 khi khởi tạo. SA0 nối GND để cố định địa chỉ theo sơ đồ; không kết luận cứ thấy 0x69 là cảm biến hỏng. Giữ nguyên đường 3,3 V, không cấp 5 V vào 3V3, CS, SDA hoặc SCL. Nhận xét về cấu trúc VIN/LDO của một module không được áp cho mọi phiên bản GY-BMI160.

## 5. Dual A4950 và hai động cơ

### 5.1. Tín hiệu điều khiển

| STM32 | Timer | Chân driver | Bánh |
|---|---|---|---|
| PA8 | TIM1_CH1 | AIN1 | Trái |
| PA9 | TIM1_CH2 | AIN2 | Trái |
| PA10 | TIM1_CH3 | BIN1 | Phải |
| PA11 | TIM1_CH4 | BIN2 | Phải |

PWM khoảng **20 kHz**, TIM1 center-aligned, giới hạn lệnh ±2499. Mỗi cặp IN1/IN2 cùng quyết định chiều và mức tác động lên một bánh; không hiểu mỗi dây là một ngõ PWM tiến/lùi độc lập. Mã motor hiện dùng slow decay: khi chạy, một đầu vào giữ HIGH và đầu còn lại được điều chế; lệnh duty bằng 0 đặt cả hai HIGH để phanh.

### 5.2. Bảng đầy đủ các chân module đang được mô tả

Quy ước hàng dưới đây kế thừa tài liệu đi dây của module đang dùng: hàng 1 ở cạnh có tụ, hàng 2 đối diện. **Ưu tiên nhãn in trên module** khi truy dây; đây không phải pinout chung cho mọi bo Dual A4950.

| Hàng | Nhãn chân | Điểm nối trong sơ đồ đã lưu |
|---|---|---|
| 1 | VM | WAGO 1 (+), nguồn pin |
| 1 | GND | WAGO 1 (−), dây nguồn riêng |
| 1 | VCC | WAGO 2 (+), 5 V |
| 1 | AIN2 | STM32 PA9 |
| 1 | AIN1 | STM32 PA8 |
| 1 | BOUT1 | Dây đỏ M1 motor phải |
| 1 | BOUT2 | Dây trắng M2 motor phải |
| 2 | AOUT2 | Dây trắng M2 motor trái |
| 2 | AOUT1 | Dây đỏ M1 motor trái |
| 2 | BIN1 | STM32 PA10 |
| 2 | BIN2 | STM32 PA11 |
| 2 | GND | WAGO 2 (−), dây riêng theo sơ đồ lưu |

Giữ nguyên đường GND đã đấu của xe đang hoạt động. Không suy ra hai chân GND trên module là hai miền cách ly hoặc mỗi chân chỉ mang một loại dòng nếu chưa đối chiếu đường đồng của bo.

### 5.3. Dây motor và encoder

Màu dây khớp file `thông số động cơ.txt`. Số pin là quy ước của tài liệu đã lưu; không đếm giắc từ một hướng tùy ý khi chưa xác định đầu pin 1.

| Dây | Chức năng | Motor/encoder trái | Motor/encoder phải |
|---|---|---|---|
| Đỏ — pin 1 | M1, cuộn dây motor | AOUT1 | BOUT1 |
| Trắng — pin 6 | M2, cuộn dây motor | AOUT2 | BOUT2 |
| Đen — pin 2 | GND encoder | Ray GND | Ray GND |
| Xanh dương — pin 5 | VCC encoder | 3,3 V từ STM32 | 3,3 V từ STM32 |
| Vàng — pin 3 | Encoder kênh A | PA0 — TIM2_CH1 | PB4 — TIM3_CH1 |
| Xanh lá — pin 4 | Encoder kênh B | PA1 — TIM2_CH2 | PB5 — TIM3_CH2 |

Dây đỏ/trắng là hai đầu cuộn motor nối đầu ra cầu H, không phải các dây cấp nguồn Hall. Đi cặp dây motor sát nhau, tránh bó sát dây I2C; không cần đổi lại bó dây đang ổn định chỉ để đạt một số vòng xoắn quy định.

### 5.4. Chiều đã chốt trên xe

- Người dùng đã xác nhận **PWM dương ở cả hai bánh làm đỉnh bánh quay về WAGO nguồn pin**, tức phía trước xe.
- Nút thử TRÁI/PHẢI đã được xác nhận đúng nhãn.
- Firmware đảo dấu encoder phải trong `encoder.c`; ticks phải thô trong frame vẫn theo dấu phần cứng, khác dấu vận tốc phải đã xử lý.
- Encoder hiện dùng 898,225 ticks/vòng bánh, từ số đo thực; không dùng lại 937,2 danh nghĩa để suy luận dây đấu sai.

**Giữ nguyên cực dây motor, thứ tự A/B encoder và hướng IMU của cấu hình đang chạy tốt.** Nếu phải tháo lắp, đối chiếu lại nhãn và kiểm tra cả chiều motor lẫn dấu encoder bằng bài test kê cao trước khi ARM trên sàn. Không tự đảo AOUT/BOUT hoặc đổi A/B chỉ vì một giá trị raw có dấu khác mong đợi.

## 6. Còi và LED

| Tín hiệu | Kết nối / hành vi firmware |
|---|---|
| Còi | PB12, HIGH để bật, LOW để tắt |
| LED onboard | PC13, active LOW |

**Người dùng xác nhận còi thực tế có hai chân:** chân dương (+) nối trực tiếp STM32 PB12, chân âm (−) nối GND. Không có dây VCC 5 V và chân IN riêng của module còi ba chân. Sơ đồ KiCad dùng BZ1 để thể hiện đúng cách đấu này. Chưa có thông số dòng tiêu thụ và loại còi để đánh giá tải trực tiếp của PB12; cần xác minh khi chốt phần cứng PCB, không tự coi cách đấu hiện tại là phù hợp cho mọi còi hai chân.

## 7. Kết nối nạp STM32 bằng ST-LINK

Dùng các chân SWD có sẵn trên Black Pill để nối mạch nạp bên ngoài; không thêm đầu nối ST-LINK riêng trên PCB mới theo yêu cầu người dùng. Các chân SWD có sẵn không đồng nghĩa bo Black Pill tích hợp mạch ST-LINK. ST-LINK nối SWD, không chiếm các chân PA8–PA11 điều khiển motor.

| Chân nạp | STM32 Black Pill |
|---|---|
| SWDIO / DIO | PA13 — SWDIO |
| SWCLK / CLK | PA14 — SWCLK |
| GND | GND |
| RST, nếu sử dụng | NRST |

Dây nguồn/tham chiếu điện áp phụ thuộc đúng loại ST-LINK đang dùng: phân biệt ngõ **3,3 V cấp ra** với ngõ **VTref đo nguồn target**. Không đấu song song ngõ cấp 3,3 V của mạch nạp với nguồn 3,3 V đang có trên xe. Không áp quy tắc bỏ VTref cho mọi loại probe.

Giữ cấu hình nạp ST-LINK/SWD trong CubeIDE; không cần tháo PA11 khi dùng SWD. PA11 đồng thời là USB D−, nên USB/DFU có thể xung đột với kết nối driver; tài liệu này không thay phương thức nạp đang sử dụng sang DFU.

## 8. Đối chiếu khi bảo trì hoặc lắp lại

Xe đang hoạt động ổn định không cần chạy lại quy trình này chỉ vì tài liệu được cập nhật. Khi thực sự tháo/lắp lại dây:

1. **Tắt nguồn** trước khi kiểm tra thông mạch, cực tính và chạm chập. Đối chiếu nhãn module và ảnh/dấu dây trước khi tháo; kiểm tra ray breadboard có bị ngắt giữa hay không.
2. **Đo điện áp khi đã cấp nguồn, motor ngắt khỏi driver:** XL4015 và ray 5 V xấp xỉ mức đặt 5,0 V; 3V3 BMI160 và CS xấp xỉ 3,3 V; SA0 gần 0 V. Không dùng khoảng 3,28–3,30 V làm tiêu chí đạt/trượt cứng cho mọi đồng hồ và LDO.
3. Nếu kiểm tra lỗi reset khi motor chạy, đo nguồn tại ESP32/STM32 trong điều kiện có tải. Điện áp tĩnh tại XL4015 không loại được sụt áp ngắn khi khởi động hoặc đảo chiều motor.
4. Khi lắp lại hoàn tất, thử bánh kê cao bằng chức năng pulse hiện hành, xác nhận đúng bánh, đúng chiều và dấu encoder. Sau đó mới thực hiện hiệu chuẩn/ARM theo quy trình trên web. Cấp pin đơn thuần không tự ARM và không tự chạy bài motor.

## 9. Phạm vi xác nhận và nguồn đối chiếu

Các chân tín hiệu, timer, UART, tần số vòng điều khiển và cách đọc cảm biến đã được đối chiếu với mã nguồn. Bố trí tầng, vị trí driver, hướng WAGO và chiều bánh dựa trên thông tin người dùng đã chốt trong `HANDOFF.md`.

Bảng nguồn, hai hàng chân module, màu dây và cực motor kế thừa sơ đồ đi dây đã lưu. Chưa có kiểm tra trực tiếp từng dây bằng ảnh mới hoặc đo thông mạch trong lần cập nhật này. Những chi tiết phụ chưa được xác nhận riêng gồm thông số dòng/loại còi và sơ đồ nội bộ module driver. INT1 → PB2 đã được người dùng xác nhận còn cắm. Người dùng đã xác nhận còi hai chân (+ → PB12, − → GND), xe không lắp tụ bổ sung. Các ghi chú đó không yêu cầu thay cấu hình xe đang hoạt động ổn định.

Nguồn trong project:

- `HANDOFF.md`; `thông số chi tiết linh kiện.txt` cho bố trí, có ưu tiên thông tin đã chốt mới nhất trong HANDOFF.
- `thông số động cơ.txt` cho màu và chức năng dây motor/encoder.
- `Xe_Can_Bang/Core/Src/tim.c`, `i2c.c`, `usart.c`, `gpio.c`, `motor.c`, `encoder.c`, `bmi160.c`, `robot_fsm.c`, `main.c`.
- `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` và `firmware_build.h`.

Tài liệu hãng dùng để phân biệt đặc tính chip với đặc tính module:

- [Bosch BMI160 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf).
- [Allegro A4950 datasheet](https://www.allegromicro.com/-/media/Files/Datasheets/A4950-Datasheet.ashx).
