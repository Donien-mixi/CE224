# RT16 STABLE — phân tích và sửa điều phối R12 ngày 03/11/2026

Firmware để nạp thử: STM32 `BAL-20261103-R12-8DIR-RT16-STABLE`, ESP32 `TWIP-WEB-20261103-BAL-R12-8DIR-RT16-STABLE-NET15`. Đây là bản sửa dựa trên RT15, chưa được kiểm chứng trên xe. Không thay thiết kế cơ khí, gain cân bằng R12, observer, hiệu chuẩn góc, PPR, quy ước chiều, ma sát/phân bổ lực từng bánh hoặc trần PWM.

## Đã đọc gì và kết luận từ log

Đã đọc toàn bộ bảy TXT (3)(1), (4)(1), (5)(1), (6), (7)(1), (8)(1), (9)(1) trong Downloads. Các lần tải lại chứa các lượt cũ: (4) có lại lượt trong (3), (8) có lượt (7), (9) có lượt (7) và (8). Có **7 lượt chạy độc lập** sau loại trùng, không phải cộng tất cả những đoạn xuất hiện trong các file. Thông báo tiến độ ban đầu ghi 8 lượt là đếm nhầm.

| Lượt xuất hiện lần đầu | Thời gian motor chạy | Lỗi dừng | Pitch khi dừng | Diễn giải |
|---|---:|---:|---:|---|
| (3)(1) | 9,34 s | 4096 | +3,14° | Hủy do bám điểm đặt, đúng 4 giây sau bị cắt motor |
| (4)(1), lượt mới | 120,28 s | 8192 | −8,33° | Kích hoạt bảo vệ nghi va chạm/chuyển động mạnh |
| (5)(1) | 34,34 s | 32 | +1,75° | Đọc I2C lỗi khi thân còn gần thẳng |
| (6) | 15,24 s | 32 | +7,62° | Đọc I2C lỗi khi xe đang chạy lùi |
| (7)(1) | 6,34 s | 4096 | +2,28° | Hủy do bám điểm đặt, hết 4 giây dù thân đã gần thẳng |
| (8)(1), lượt mới | 16,42 s | 8 | +4,96° | Đạt ngưỡng quãng phanh cũ khi còn giảm tốc |
| (9)(1), lượt mới | 28,84 s | 8192 | +1,39° | Gyro/gia tốc kích hoạt bảo vệ chuyển động mạnh |

Khoảng thời gian tính từ mẫu cân bằng đầu đến cuối, sai số lấy mẫu khoảng 20 ms. Góc nhỏ ở mẫu ngắt không đủ chứng minh xe không bị va chạm: CF có độ trễ; log chỉ lấy 50 Hz từ điều khiển 200 Hz. Chưa thể xác định chắc chắn tác động va chạm, xe đang ngã, lỗi cơ khí hay bất thường cảm biến là tác nhân đầu tiên của hai lần 8192.

Không thấy CRC lỗi, mất khung, tràn hoặc STM32 reset trong các header. Chu kỳ lưu được 4.999–5.014 ms, thời gian xử lý lớn nhất trong các mẫu đang cân bằng 652 µs. Điều này không chứng minh mọi chu kỳ 200 Hz đều như vậy và không đo được toàn bộ độ trễ từ ngón tay đến motor.

**PWM đã chạm ±2499 trên cả hai bánh**, đặc biệt các lượt (4) và (5). Tốc độ tâm đo lớn nhất khoảng **93,36 cm/s**; lượt (5) tiến tối đa khoảng 88,28 cm/s, lùi khoảng 93,36 cm/s. Không có căn cứ nói PWM chưa tới đỉnh hoặc tăng giới hạn PWM vượt 2499. Chạm đỉnh thoáng qua cũng không đồng nghĩa đã đo được tốc độ cực đại liên tục của motor có tải. Không ghi nhận điện áp/dòng trực tiếp.

Biểu đồ và số liệu chi tiết ở `output/RT16/phan_tich_log_RT15.png`, `output/RT16/mo_phong_RT16.png`, `output/RT16/metrics.json`; các tệp này phục vụ phân tích, không cần để nạp xe.

## Các lỗi phần mềm tìm được và thay đổi

### Kết thúc hủy lệnh và phanh

`BalanceMotion8_Update` trước đây chỉ xác nhận kết thúc hủy khi cờ `release_required` còn bật. Nhưng một gói trung tính mới lại xóa cờ đó trước phép xác nhận. Vì thế nhả nút có thể khiến đồng hồ hủy còn tồn tại đến 4 giây dù thân đã yên. Hai lượt lỗi 4096 đã có các đoạn đáp ứng điều kiện yên mới dài lần lượt khoảng 2,98 s và 2,16 s trước khi bị ngắt; các đoạn này được đối chiếu ở 50 Hz, không phải phát lại đầy đủ vòng kín 200 Hz.

RT16 hoàn tất hủy theo **cờ hủy + trạng thái vật lý**, độc lập với việc trung tính đã được nhận hay chưa. Xác nhận liên tục 500 ms: pitch ≤6°, gyro <25°/s, tốc độ tâm <0,10 m/s, tốc độ quay encoder <0,30 rad/s, tham chiếu vận tốc/quay gần 0. Khi kết thúc mới chốt điểm giữ hiện tại, đóng cổng lệnh, yêu cầu trung tính mới rồi bấm mới; không tự chạy lại lệnh cũ. Readiness thông thường không được vượt qua một episode hủy chưa hoàn tất.

Phanh nhả nút xác nhận dừng sau 100 ms với tốc độ <0,06 m/s, pitch ≤6°, gyro <25°/s, tham chiếu gần 0. Ngưỡng góc cũ 3° quá hẹp so với góc cân bằng thực quan sát được. Không reset encoder hoặc observer khi nhả/bấm/khôi phục.

Quãng phanh từ 0,95 m/s với mức giảm tốc tham chiếu 0,65 m/s² đã là khoảng 0,69 m **trước** jerk, độ trễ bám tốc và bước giữ cân bằng cuối. Lượt (8) đạt gần 1 m khi còn lùi 0,144 m/s. RT16 dành **1,25 m/4 s cho phanh nhả nút thông thường**. Hủy do kẹt/khôi phục vẫn 1 m/4 s; ngân sách góc khởi động và REC1 không thay. Đây là thay đổi giới hạn có chủ đích, không phải cam kết phanh tức thời hoặc mọi trường hợp đều dừng trong 1,25 m.

### Tiến bị ghìm hơn lùi

Bộ tạo tham chiếu RT15 xét góc dự báo có dấu. Khi xe cân bằng ở pitch dương vài độ, gate thấp 5–6° có thể ghìm riêng chiều tiến, giữ tham chiếu thấp và cuối cùng gây hủy do xe không theo điểm đặt. Đây không tự chứng minh motor trái yếu hoặc encoder sai.

Ở tốc độ tham chiếu dưới 0,50 m/s, RT16 áp dụng cho cả ba preset: ngưỡng dự báo vào hạn chế 8°; thoát khi góc dự báo dưới 5,5°, pitch dưới 6°, gyro dưới 25°/s và có dự trữ PWM/bám tốc trong 100 ms. Ngưỡng PWM 90%, sai lệch tốc, giới hạn gia tốc/jerk, điều kiện tốc độ cao và bảo vệ góc vẫn còn. Không ép PWM lên cực đại để đuổi 95 cm/s. Đây là nới vùng dự báo khởi tốc, không đổi mốc góc cảm biến hoặc gain giữ cân bằng.

### Encoder và điểm đặt

Đã kiểm tra timer TIM2/TIM3, chiều delta, xử lý vòng đếm, LPF và các vị trí reset. Dấu giữ nguyên: quay tiến thì tick thô trái tăng, phải giảm; vận tốc xử lý hai bên cùng dương. LPF có hệ số giữ mẫu cũ 0,75; lượng tử encoder ở tốc độ thấp là có thật.

Trước đây bộ giám sát tích phân vận tốc đã lọc để lấy vị trí, còn luật R12 dùng vị trí trực tiếp từ tick. Hai phần có thể lệch khoảng 1,5 cm ở MAX95 do độ trễ LPF. RT16 dùng vị trí tick cho cả hai; vận tốc vẫn lọc như cũ. Không sửa dữ liệu đo để hai bánh trông bằng nhau. Kiểm thử rollover và kênh độc lập không thay thế được đo nhiễu dây encoder, độ trượt bánh hoặc đồng tâm bánh ngoài thực tế.

Bộ bù hướng R12 được giữ nguyên, không thêm bù PWM cố định từ những lượt vừa rẽ, vừa phanh, vừa gặp lỗi. Chưa đủ dữ kiện tách sai số bán kính lăn, trượt sàn, ma sát hộp số và lệch đo encoder để cam kết xe thực chạy thẳng tuyệt đối. Chuỗi điểm đặt/phanh nhất quán giúp bộ giữ hướng hiện có làm việc đúng hơn; hiệu quả thực còn phải đối chiếu log mới.

### BMI160

- Kiểm tra kết quả **mọi lệnh ghi cấu hình**, đọc lại dải đo/ODR và PMU normal. Chờ gyro 85 ms, phù hợp mốc 80 ms của Bosch SensorAPI; không tăng thời gian trong vòng điều khiển.
- Kiểm tra BUSY trước giao dịch; cho STOP bình thường tối đa 20 µs để hoàn tất. Không để HAL chờ BUSY mặc định 25 ms trong vòng 5 ms.
- Chỉ thử lại **một lần khi NACK ngắn dưới 750 µs**; lần đọc lại có timeout 1 ms. Không dùng mẫu cũ/giá trị 0 làm mẫu hợp lệ, không reset bus hoặc hiệu chuẩn trong ngắt.
- Mất mẫu không khắc phục được vẫn dừng motor. Khi đã dừng, nút ĐO LẠI GYRO mới thực hiện bus clear open-drain tối đa 9 xung SCL, tạo STOP, khởi tạo lại ngoại vi/cảm biến, xác minh cấu hình và hiệu chuẩn. Sau đó cần BẬT CÂN BẰNG mới; không tự chạy lại.
- Web chi tiết và BINFO thêm số lỗi đọc, số NACK thử lại thành công, mã lỗi gần nhất và thời điểm STM32. Bộ đếm lỗi/thử lại bão hòa 65535; mã bit31 = bus/HAL đang BUSY, các bit thấp theo HAL I2C. Đây là thống kê từ lúc cấp nguồn, không phải mỗi lượt chạy. Metadata cuối không mô tả mọi sự kiện từng mẫu trong lượt cũ.

Lỗi 32 của log cũ chưa có mã HAL, nên chưa biết là NACK, bus kẹt, dây lỏng hay mất nguồn cảm biến. Firmware không thể khắc phục dây đứt hoặc nguồn biến mất sau ngã. Không quy mọi lỗi I2C cho ngã: lượt (5) lỗi khi pitch chỉ 1,75°.

## Kiểm chứng đã thực hiện

- Kiểm thử mã điều phối: trung tính trong hủy, không tự tiếp tục lệnh cũ, bấm lại, phanh ở góc cân bằng thực, giới hạn chạy trôi thật hai chiều và khởi tốc có lệch góc. Bài kiểm tra kết thúc hủy **thất bại với snapshot RT15**, qua với RT16.
- Driver BMI160 thật với HAL giả lập: ghi lỗi, đọc lại sai, PMU, NACK đơn/kép, NACK kéo dài, BUSY, SDA kẹt có thể giải phóng và SCL kẹt không thể giải phóng.
- Encoder thật với bộ đếm giả lập: đi qua vòng đếm 16 bit hai chiều, trái/phải độc lập, không thêm tick khi đứng yên, vận tốc lọc trở về 0.
- Các kiểm thử production FSM, parser/phiên lệnh, dashboard, HTTP tải log, bộ tạo tham chiếu và REC1 đều qua. Header cân bằng R12 gốc vẫn giống bản nguồn mô phỏng; frame UART hai MCU vẫn giống nhau, 122 byte/40 cột CSV.
- Build STM32 và ESP32 thành công, chưa nạp lên xe bằng công cụ trong phiên này.
- Scilab chạy 12 ca tải/tốc độ cộng 12 ca biên, mỗi ca 20 s: **24/24 đủ thời gian, không kích hoạt guard**. Gồm tiến/lùi, đảo hướng, đường chéo, quay hai chiều, nhả/bấm nhanh, VM 11,2 V, giảm lực 20%, lệch motor ±5%, độ trễ tăng, sai mốc góc ±2° và lệch trọng tâm 3 mm. Không thay kích thước/bố trí danh định để làm đẹp kết quả.
- Loạt tốc độ: pitch lớn nhất khoảng 11,00°; quãng phanh lớn nhất đo trong các ca khoảng 0,84 m. Ca quay tại chỗ cho hai chiều khoảng ±126°/s rồi về gần 0 khi nhả. Đây là mô hình có giả định ma sát/lực; không phải số đo bảo đảm của xe.
- Mô phỏng vẫn chưa đạt 95 cm/s liên tục ở mọi ca, một số ca giữ khoảng 80 cm/s do giới hạn dự trữ. Mô hình không tái hiện mọi lỗi firmware của RT15 chỉ bằng một ca danh định; phải thêm kiểm tra chuyển trạng thái và chèn lỗi cảm biến. 24 ca không chứng minh chạy vô hạn hoặc chịu mọi va chạm.

Đường dẫn kết quả: `Simulation/results/runs/20261103_185422_152_motor_catalog_rt16_final_stress` và `20261103_185406_187_motor_catalog_rt16_final_edges`. Mỗi thư mục có CSV quỹ đạo/guard. Các kết quả phát sinh không cần để nạp firmware.

## Cách chạy lại và test tiếp

1. Nạp project STM32 `Xe_Can_Bang` và sketch `ESP32_S3_Gateway/ESP32_S3_Gateway.ino`. Kiểm tra đúng hai chuỗi phiên bản RT16/NET15 trên web; không dùng bản bin ESP32 cũ trong thư mục export trước đây.
2. Kết nối Wi-Fi của xe, mở `http://192.168.4.1/ui/r12?v=RT16-NET15`. Cố định thân, đo lại gyro; đặt xuống sàn đủ thoáng, bật cân bằng. Trước hết kiểm tra đứng yên, tiến/lùi ở 50 và bấm–nhả ngắn, quay hai chiều. Sau khi những bước này tốt mới thử 72/95; chừa quãng phanh lớn hơn trước. Không cố tình cho xe va tường hoặc ngã mạnh để kiểm tra I2C.
3. Nếu dừng lỗi, chờ 2 s rồi tải log trước khi đo lại gyro/cắt nguồn. Ghi lại lệnh cuối đang giữ hay đã nhả, xe có va chạm không, hướng trôi và cảm nhận phản ứng. Có thể gom nhiều lượt trong một TXT, log có state/epoch/sequence để tách.
4. Với lỗi BMI160, xem mã I2C trong Chi tiết; sau khi đã tải log thì cố định xe rồi ĐO LẠI GYRO. Không cần cắt nguồn nếu bus clear thành công. Nếu vẫn lỗi, kiểm tra tiếp xúc/nguồn/SCL/SDA/CS/SA0; không bỏ bảo vệ để chạy thiếu IMU.

Mô phỏng hiện hành: Execute `robot_can_bang_pid_1.sce` ở gốc bằng Scilab có giao diện. Launcher dùng Node + GCC để biên dịch wrapper từ header STM32 hiện tại, chạy 12 ca tốc độ và vẽ biểu đồ; không thoát Scilab. Cần Node trên PATH và GCC tại `C:/msys64/ucrt64/bin/gcc.exe`. Chạy không giao diện: `node tools/run_governor_sim.cjs --stress`; bộ ca biên: thêm `--edge`. Loạt mới bật vị trí giám sát từ tick; kết quả cũ không bị viết lại. `Simulation/current/review.sce` là bộ tổng quan cũ không tích hợp đầy đủ governor, không dùng nó để kết luận đạt MAX95/RT16.

## Nguồn kỹ thuật đối chiếu

- [Bosch BMI160 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf): thanh ghi, PMU, dải đo và quy trình giao tiếp.
- [Bosch SensorAPI, định nghĩa thời gian chờ](https://github.com/boschsensortec/BMI160_SensorAPI/blob/master/bmi160_defs.h): `BMI160_GYRO_DELAY_MS=80`, accel 5 ms.
- HAL STM32F4 đang đi kèm project: `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c`, kiểm tra thực tế `I2C_TIMEOUT_BUSY_FLAG=25` và hành vi timeout. Không đổi HAL thư viện.

Ưu tiên lượt tiếp theo là kiểm chứng khả năng nhả–dừng và lỗi I2C trên xe. Chưa có cơ sở khẳng định tất cả vấn đề cơ khí/tín hiệu sau tháo lắp đã được loại bỏ, hoặc mọi bảo vệ dừng đều là bug.
