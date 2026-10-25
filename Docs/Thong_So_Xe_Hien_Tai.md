# Thông số xe và mô hình hiện hành

Snapshot bàn giao R10S1; các giá trị số thực thi nằm trong mã/config. Tài liệu này không phải dữ liệu đo mới.

Hiện tại mô phỏng dùng **thiết kế xe 888 g**, vòng điều khiển **R10**, cùng quy trình khởi động **R10S1**. Dưới đây là các thông số đang dùng; những giá trị suy tính hoặc giả định được ghi rõ.

**Thiết kế cơ khí**

| Thông số | Giá trị trong mô phỏng |
|---|---:|
| Tổng khối lượng xe | **888 g** |
| Khối lượng thân, gồm motor và linh kiện | 788 g |
| Hai bánh và khớp nối | 100 g |
| Hai motor GA25 | 294 g, chia đều **147 g/motor** |
| Đường kính bánh | **65 mm** |
| Bán kính lăn | 32,5 mm |
| Kích thước mỗi tấm mica | 80 × 200 mm |
| Độ dày mica | 1,6 mm |
| Số tấm mica | 3 |
| Khoảng cách mặt trên giữa các tấm | 55 mm |
| Mặt trên mica 1/2/3 so với trục bánh | 22,5 / 77,5 / 132,5 mm |
| Cao độ tâm chip BMI160 so với trục bánh | **89,5 mm** |
| Khoảng cách tâm hai bánh | **262 mm — suy từ hình học, chưa đo xác nhận** |

Mô phỏng hiện đặt cao độ trục so với đất bằng **32,5 mm**, không còn dùng số đo khoảng 35 mm trước đây.

Bố trí chính: pin và XL4015 ở tầng dưới; breadboard, STM32, ESP32, **A4950 và BMI160 ở tầng trên**. WAGO nguồn pin là phía trước; WAGO nguồn 5 V là phía sau.

**Khối lượng linh kiện và trọng tâm**

| Cụm | Khối lượng |
|---|---:|
| Ba tấm mica | 95 g |
| Pin LiPo 3S | 190 g |
| XL4015 | 15 g |
| Breadboard | 77 g |
| STM32 / ESP32 | 7 / 10 g |
| Hai A4950 / BMI160 | 3 / 2 g |
| Hai WAGO | 12 g |
| Dây trên breadboard | 15 g |
| Hai motor | 294 g |
| Hai gá motor | 20 g |
| Tám cọc | 48 g |
| Hai bánh và khớp nối | 100 g |

Từ cách phân bố này, mô hình tính được:

| Đại lượng | Giá trị |
|---|---:|
| Trọng tâm thân cao hơn trục bánh | **36,933 mm** |
| Trọng tâm toàn xe cao hơn trục bánh | 32,774 mm |
| Lệch trọng tâm trước/sau | **0 mm — giả định phân bố đối xứng** |
| Lệch trọng tâm ngang | Khoảng 1,739 mm |
| Mô-men quán tính thân quanh trọng tâm, theo trục nghiêng | 0,001368297 kg·m² |
| Mô-men quán tính thân quanh trục bánh | 0,002443180 kg·m² |

**Các giá trị trọng tâm và quán tính là kết quả tính từ mô hình bố trí, chưa phải phép đo độc lập trên xe.** Một số kích thước linh kiện và cách chia khối lượng vẫn là giả định.

**Motor và encoder**

| Thông số | Giá trị |
|---|---:|
| Motor | GA25-370, danh nghĩa 12 V |
| Tốc độ không tải theo tài liệu | 280 vòng/phút |
| Tỉ số truyền theo tài liệu | 21,3:1 |
| Mô-men định mức theo tài liệu | 0,4 kgf·cm/motor |
| Mô-men tối đa theo tài liệu | 2 kgf·cm/motor |
| Điện áp pin dùng tính toán | **11,1 V — danh nghĩa, chưa đo khi chạy** |
| Encoder trái đo được | 898,450 ticks/vòng bánh |
| Encoder phải đo được | 898,000 ticks/vòng bánh |
| PPR dùng trong bộ điều khiển | **898,225 ticks/vòng** |
| Lọc tốc độ encoder | β = 0,75 |
| Giới hạn PWM | ±2499 |

Bù vùng chết đang dùng:

| Chiều lệnh | Trái | Phải |
|---|---:|---:|
| PWM dương, chạy về trước | 160 | 200 |
| PWM âm, chạy về sau | 200 | 140 |

Đường tốc độ không tải được lấy từ log thực. **Lực dưới tải, ma sát và thời gian đáp ứng mô-men vẫn chưa được đo đầy đủ.**

Trong đợt kiểm tra khởi động mới nhất, mô phỏng khảo sát ba giả định:

| Trường hợp | Hằng số thời gian lực motor | Hệ số lực so với mô hình gốc | Trễ quan sát bổ sung |
|---|---:|---:|---:|
| 1 | 3 ms | 0,75 | 20 ms |
| 2 | 5 ms | 0,50 | 20 ms |
| 3 | 12 ms | 0,75 | 30 ms |

Đây là **các tình huống kiểm tra độ nhạy**, không phải ba thông số đã đo được của xe.

**BMI160 và xử lý góc**

| Thông số | Giá trị |
|---|---:|
| Tần số cập nhật cảm biến | 100 Hz |
| Chu kỳ điều khiển | **5 ms, tương đương 200 Hz** |
| Thang gia tốc | ±8 g, 4096 LSB/g |
| Thang gyro | ±2000°/s, 16,4 LSB/(°/s) |
| Bộ lọc bù góc | α = **0,98** |
| Mốc góc lắp đặt đã trừ | 2,166943° |
| Trim điều khiển | 0° |
| Quy ước góc điều khiển | Nghiêng về WAGO nguồn pin là dương |

Hiệu chỉnh gia tốc kế dùng công thức `(raw_g − bias)/scale`:

| Trục | Bias, g | Scale |
|---|---:|---:|
| X | −0,041683610 | 0,997272838 |
| Y | −0,105884357 | 1,007589777 |
| Z | +0,004738195 | 1,009293880 |

Mô phỏng còn giữ sai số góc dư khoảng **±0,529812°** để kiểm tra bất định. Các hệ số hiệu chuẩn này đang được sử dụng, nhưng bộ dữ liệu trước đó vẫn có cảnh báo tư thế/chuyển động nên chưa coi là hiệu chuẩn vật lý hoàn toàn được xác nhận.

**Bộ điều khiển R10 hiện hành**

| Thành phần | Hệ số |
|---|---:|
| Kp góc | **48** |
| Kd góc | **3,6** |
| Kp vận tốc | **32** |
| Ki vận tốc | **0,5** |
| Kp vị trí | **0,3** |
| Giới hạn góc mục tiêu | ±8° |
| Giới hạn tích phân vận tốc | ±2° |
| Bắt đầu cho phép tích phân | Sau 3 giây |
| Bù chống xoay | Kp = 500; Kd = 40; giới hạn **±8 PWM** |

Phản hồi góc giữ nguyên trong vùng ±3°; tăng dần tới **1,75 lần** khi góc đạt ±6°. R10S1 giữ nguyên phần điều khiển này.

**Điều kiện bắt đầu R10S1 và bảo vệ**

- Góc trong **±2°**, có kiểm tra dự báo trước 0,15 giây.
- Gyro mỗi trục dưới **3°/s**; tốc độ mỗi bánh dưới **0,02 m/s**.
- Góc lọc và góc accel chênh không quá **0,75°**.
- Giữ điều kiện **0,3 giây**; chờ thêm **1,5 giây** sau hiệu chuẩn.
- Ngắt khi góc vượt **25°**, quãng encoder vượt **30 cm**, hoặc tốc độ vượt **0,8 m/s**.

Nguồn cấu hình: [mô hình cơ khí và cảm biến](../Simulation/model/robot_can_bang_pid_1.sce), [cấu hình lượt mô phỏng nhóm](../Simulation/config/team_run.sce), [thông số firmware](../Xe_Can_Bang/Core/Inc/balance_profile.h).

**Phần còn thiếu chính để mô phỏng bám sát xe là đáp ứng motor dưới tải và kiểm chứng độc lập trọng tâm, khoảng bánh, chu vi lăn.** Hiện mô hình chưa tái hiện đầy đủ những lượt R10 trôi rồi ngã ngoài thực tế.