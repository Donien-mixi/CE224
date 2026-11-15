> **Trạng thái:** bản hiện hành **RT29-NOCUT / NET30** — xem [tổng quan](Tong_Quan_Do_An.md) và [HANDOFF](../HANDOFF.md).

> Các mô tả theo phiên bản cũ bên dưới là tài liệu đối chiếu. Dùng tổng quan phía trên để xác định cấu hình hiện hành.

# Chức năng xe hiện hành — RT19 HOLD80 / NET18, 08/11/2026

RT19 sửa hai lỗi đọc từ log xe thật (13)(2): mốc hướng bị bám theo thân khi còn braking sau quay (xe lệch trái +59° trong 4,9 s lệnh thẳng) và mốc vị trí bị chốt quá sớm trước khi hết lăn ngược (sai số cuối 7–10 cm kèm lực đẩy thừa). Mức cao nhất đổi từ 95 xuống **80 cm/s** vì log đo được PWM2499 chỉ đạt ~0,96 m/s. Không đổi gain/observer/phân bổ lực R12, con quay, encoder. 12/12 ca thường, 12/12 khởi hành50, 6/6 biên đạt 20 s. Chưa có log RT19 trên xe. Web `http://192.168.4.1/ui/r12?v=RT21-NET20`.

RT17 giữ nền R12 và các chức năng dưới; xem `RT17_Tien_Lui_Va_PhuC_Hoi.md`. Bản đã build/chưa thử xe. Mới: mốc tư thế cho predictor học từ lúc tự đứng yên (không đổi góc IMU/gain), hủy xác nhận200ms không chờ readiness500ms thêm, đảo chiều bỏ gia tốc lưu sai hướng; REC1 nhận lệnh mới ở mức25cm/s khi thân đã ổn, sau REC lượt giữ cũ tối đa50cm/s (thao tác mới trở lại mức chọn), flag32768; 250dps+norm bất thường thử REC1, >900dps vẫn cắt. Hiệu chuẩn lỗi giữ bias tốt nhưng khóa ARM. 24ca thường/biên qua, các bài xô thử thách vẫn có ca ngã; không cam kết mọi tình huống cực đoan.

Các ghi chú RT16 ngay dưới là nền trước RT17; dwell hủy hiện là200ms và không còn bước readiness lặp.

RT16 đã build và kiểm thử mô phỏng/host, chưa có log chạy thật sau sửa. Xem `RT16_Phan_Tich_7_Log_Va_On_Dinh.md`. Các chức năng dưới đây được giữ; RT16 bổ sung:

- Hoàn tất hủy theo xe thực sự yên, không bị kẹt đồng hồ 4 giây sau khi đã nhận nhả nút; vẫn cần trung tính/bấm mới sau hủy.
- Xác nhận phanh ở pitch ≤6°; phanh nhả nút riêng có ngân sách 1,25 m/4 s, hủy do kẹt vẫn 1 m/4 s. Không giới hạn tổng hành trình chạy chủ động.
- Vị trí giám sát lấy từ tick giống luật R12; không reset encoder khi đổi lệnh.
- Hysteresis góc dự báo khởi tốc thấp 8°/5,5°, mọi preset; giữ giới hạn tốc độ cao, gia tốc/jerk và lực cân bằng.
- BMI160 kiểm tra cấu hình/PMU, retry một NACK ngắn, thoát nhanh BUSY; bus clear chỉ khi đo lại gyro đã dừng. Web/log có mã I2C và số retry thành công; mất IMU thật vẫn dừng motor.

Danh sách mô tả code đang vận hành, không phải chứng nhận mọi tình huống thực tế đã đạt. Nền điều khiển vẫnR12; R7 và các trang đo hàng loạt cũ không còn là chế độ vận hành đang mở.

## Cân bằng và phục hồi

- Tự cân bằng bằng bộ phản hồi trạng tháiR12, observer và phân bổ lực riêng hai motor; không phải một PID góc đơn thuần.
- Cho phép bật cân bằng từ pitch trong±60° khi gyro/bánh yên và đủ điều kiện; pha dựng có thời gian, góc, tốc độ và ngân sách trôi riêng tối đa1m. Người dùng đã thử được60°; không bảo đảm mọi va chạm/dốc60°.
- Phục hồi nhiễu khi đang chạy, giới hạn thời gian4s và các ngân sách tạm; cân bằng được ưu tiên hơn lệnh chạy/quay. Va chạm quá mạnh có thể dừngmotor, không hứa không bao giờ ngã.
- Giữ vị trí/giữ hướng khi không điều khiển; bù chênh hai bánh bằng encoder và bộ giữ/quay hướng.

## Điều khiển web

- WiFi AP TWIP_RACER_S3, web192.168.4.1; ESP làm gateway, STM thực hiện điều khiển200Hz.
- Joystick8hướng: tiến,lùi,quay trái/phải tại chỗ và4hướng chéo; lệnh quay giữ tốc độ góc khi còn giữ, không chỉ quay một góc ban đầu.
- Ba preset 50 / 72 / **80** cm/s. Trần đo được trên sàn ở PWM2499 khoảng 0,96 m/s, nên 80 cm/s còn ~17 % dự trữ PWM; 95 cm/s cần 97–99 % duty nên không giữ được thăng bằng.
- Ramp vận tốc và giới hạn jerk, giảm gia tốc khi thiếu dự trữ PWM/góc/khả năng bám tham chiếu. RT15 chỉ tăng vùng gia tốc đã qua phép so sánh; luật cân bằng và trần tốc độ không đổi.
- Thả/đưa joystick về tâm gửineutral, phanh chuyển động rồi giữ cân bằng. RT14 loại bỏ gia tốc lưu làm tăng |vref| sau thả; RT15 giữ sửa này. Không thể làm vận tốc cơ học về0 tức thì mà bỏ qua quán tính/cân bằng.
- DỪNG CHẠY giữ cân bằng; DỪNG MOTOR ngắt tác động motor, người dùng cần đỡ xe.
- Chuyển preset, chống lệnh cũ bằng epoch/sequence, một chủ điều khiển; thời hạn lệnh chuyển động350ms. Gateway heartbeat cho phép giữ cân bằng khi không còn trang điều khiển; lệnh chạy cũ không được giữ vô hạn.

## Đo lường và chẩn đoán

- BMI160 accel±8g,gyro±2000°/s,ODR100Hz; đọc I2C trong vòng200Hz. CF0,998, hiệu chuẩn gia tốc và gyro bias; đo lại gyro từ web. Không đổi sang DMA/INT1 pipeline trong nâng cấp này.
- EncoderTIM2(PA0/PA1) trái,TIM3(PB4/PB5) phải;898,225ticks/vòng bánh65mm; dấu phải được đảo trong phần mềm.
- PWM TIM1 khoảng20kHz,±2499, slow-decay và phanh; mô hình motor từng chiều có bù ma sát/back-EMF, ưu tiên lực cân bằng.
- Web hiển thị pitch,tốc độ,quãng đường tương đối,PWM,trạng thái/phục hồi/lỗi; bảng chi tiết IMU/encoder/điều khiển; phiên bản và tình trạng kết nối.
- Ghi log50Hz khi cân bằng/chạy và2s sau dừng, gồm yêu cầu tiến/quay, sequence/thời gian STM nhận lệnh, tham chiếu, PWM,encoder/IMU và lỗi. Lưu tối đa600s mẫu hoạt động theo cấu hình dung lượng; đầy thì dừngghi, không ghi đè, điều khiển vẫn tiếp tục. Tải sau dừng và chờ2s.
- Đếm CRC,mấtframe,reset,thông tinWiFi/heap và nguyên nhân dừng. Cơ chế lưuFlash có trong source nhưng các log mới báo filesystem_unavailable_preserved; chưa được xem là lưuFlash hoạt động chắc chắn. Cần tải log trước khi cắt nguồn.

## Bảo vệ và giới hạn

Kiểm dữ liệu hữu hạn, cấu hìnhIMU,I2C,chu kỳ,hiệu chuẩn,heartbeat,góc,tốc độ,sai lệch quãng đường,phục hồi quá hạn và va chạm mạnh. Điều khiển chạy không bị giới hạn tổng hành trình60cm; vẫn có giới hạn sai lệch so tham chiếu và ngân sách dừng/phục hồi. Không tăng các giới hạn bảo vệ chỉ để đạt tốc độ.

Chưa có đoVM/dòng motor trực tiếp, cảm biến khoảng cách/tường, phát hiện vật cản đáng tin cậy, định vị tuyệt đối, tránh vật cản/cameraAI/TinyML hoặc tự lái. Trạng thái joystick không phải log toàn bộ tọa độ ngón tay. Bảng testencoder/đặt mốc đã gỡ; ticks vẫn có trong chi tiết và log.

Đấu dây hiện hành: motor trái đỏ→AOUT2,trắng→AOUT1; phải đỏ→BOUT1,trắng→BOUT2. Giữ các chân encoder/IMU và PA8..PA11 theo tài liệu đã sửa.
