> **Trạng thái:** bản hiện hành **STM32 RT34-CORNER + ESP32 NET35** — xem [chức năng](Tinh_Nang_Xe_Hien_Hanh.md) và [HANDOFF](../HANDOFF.md).

# Thông số xe và mô hình hiện hành

## Cấu hình kê pin 10 mm

Pin LiPo 3S (190 g theo mô hình) vẫn nằm ở tầng dưới. Ngày 08/11/2026, người vận hành xác nhận đã kê pin cao thêm **10 mm** so với vị trí cũ. Mô hình đã có biến `battery_riser_height = 0.010 m`, đưa tâm pin từ 35,8 lên **45,8 mm so với trục bánh**. Giữ giả định vị trí ngang của pin và bỏ qua khối lượng miếng kê cho đến khi có số đo. 888 g là tổng khối lượng mô hình, chưa cân lại sau thay đổi. Không cộng thêm 10 mm lần nữa.

Bản mã trong workspace là STM32 **RT34-CORNER** và ESP32 **NET35** (18/11/2026). Xem [HANDOFF](../HANDOFF.md) để phân biệt bản mã, bản đã thử và kết quả lịch sử. Kết quả thử cũ chỉ được quy cho cấu hình kê pin khi hồ sơ lượt thử xác nhận điều kiện này.

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

Pin được nâng 10 mm theo phương đứng, đưa tâm pin lên 45,8 mm so với trục bánh. Khối lượng phần kê được bỏ qua. Từ cách phân bố này, mô hình tính được:

| Đại lượng | Giá trị |
|---|---:|
| Trọng tâm thân cao hơn trục bánh | **39,344 mm** |
| Trọng tâm toàn xe cao hơn trục bánh | 34,914 mm |
| Lệch trọng tâm trước/sau | **0 mm — giả định phân bố đối xứng** |
| Lệch trọng tâm ngang | Khoảng 1,739 mm |
| Mô-men quán tính thân quanh trọng tâm, theo trục nghiêng | 0,001378410 kg·m² |
| Mô-men quán tính thân quanh trục bánh | 0,002598220 kg·m² |

**Các giá trị trọng tâm và quán tính là kết quả tính từ mô hình bố trí, chưa phải phép đo độc lập trên xe.** Một số kích thước linh kiện và cách chia khối lượng vẫn là giả định.

**Motor và encoder**

| Thông số | Giá trị |
|---|---:|
| Motor | GA25-370, danh nghĩa 12 V |
| Tốc độ không tải theo tài liệu | 280 vòng/phút |
| Tỉ số truyền theo tài liệu | 21,3:1 |
| Mô-men định mức theo tài liệu | 0,4 kgf·cm/motor |
| Mô-men tối đa theo tài liệu | 2 kgf·cm/motor |
| Điện áp pin dùng tính toán | **11,7 V — theo log xe 17/11** (dải 12,2 V đầy – 11,2 V cạn) |
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
| Bộ lọc bù góc | α cố định **0,998** |
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

**Bộ điều khiển R12 BASE60**

Phản hồi trạng thái STATE4_OBS14, không phải bộ PID góc/vận tốc cũ. Gain lực Kx/Kv/Kθ/Kω = `[9.150813420, 6.128855436, 14.32794546, 1.160483551]`, đầu vào SI(m, m/s, rad, rad/s), đầu ra N. Bộ ước lượng14trạng thái, blend0,25; phân bổ lực từng bánh có back-EMF/ma sát riêng; giữ hướng PI/PD giới hạn0,08Nm và tích phân0,04Nm. Không cộng thêm deadband PID vào đầu ra R12.

Khởi động đến ±60°, gyro/bánh yên200ms. Ngân sách trôi tăng theo góc, trần1m; pha dựng3giây hoặc4giây nếu bật trên58°. Sau dựng ổn ngắt ở25°/0,90m/s. Xem [chức năng hiện hành](Tinh_Nang_Xe_Hien_Hanh.md) và [bảng mã lỗi](Bang_Ma_Loi_Tu_Dong_Dung.md).

Nguồn cấu hình: `Core/Inc/phase1_catalog_control.h`, `phase1_catalog_params.h`, `balance_profile.h`, `balance_startup.h`; hình học tại `Simulation/model/robot_can_bang_pid_1.sce`. Cửa vào mô phỏng hiện tại chạy R12. Không dùng `team_run.sce` hoặc các hệ số R7 cũ để thay đổi firmware hiện hành.

**Giới hạn mô hình:** giữ bố trí và khối lượng; chưa tự suy ra CG lệch chỉ từ chiều ngã. Mô hình động cơ dưới tải/góc lớn chưa dự đoán đúng toàn bộ thực tế. Thành công khởi động60° là quan sát người vận hành; cần log/đo riêng nếu định hiệu chỉnh mô hình góc lớn tiếp.

## Đường đặc tính bánh đo từ log xe thật (17/11/2026)

Đoạn chạy thẳng giữ lệnh, `|v| > 0,35`, `PWM > 150`, pack 11,7 V; bánh trái đã trừ 110 PWM bù:

| Bánh | v = gain·PWM + intercept | v @ PWM 2499 |
|---|---|---:|
| L+ trái tiến | `0,000417·PWM − 0,0332` | 1,010 m/s |
| L− trái lùi | `0,000396·PWM − 0,0279` | 0,961 m/s |
| R+ phải tiến | `0,000394·PWM − 0,0187` | 0,965 m/s |
| R− phải lùi | `0,000432·PWM − 0,0593` | 1,019 m/s |

Suy ra: trần không tải **~0,96 m/s** ở 11,7 V; mức 0,80 cần duty ~0,82, mức 0,90 cần ~0,88–0,92.
Bốn hệ số này được nạp vào `Simulation/config/motor_pair_r8_20261011.sce` (xem
[RT35](RT35_Mo_Phong_Khop_Log.md)).