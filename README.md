# Xe hai bánh tự cân bằng — R12 RT8 + NET5

STM32 `BAL-20261024-R12-8DIR-RT8`; ESP32 `TWIP-WEB-20261024-BAL-R12-8DIR-RT8-NET5`. RT8/NET5 cần nạp cả hai board; xem [kế hoạch, thay đổi và cách kiểm chứng](Docs/Ke_Hoach_Sua_Loi_RT8.md). Bổ sung [chẩn đoán reset, giảm tải mạng và log Flash dự phòng](Docs/On_Dinh_Ket_Noi_NET1.md); không đổi bộ cân bằng.

- Mở project `Xe_Can_Bang/` trong STM32CubeIDE, Build rồi nạp ST-LINK.
- Mở `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` trong Arduino IDE; cấu hình board trong README cùng thư mục.
- Mô phỏng: xem `Simulation/README.md`, sau đó Execute `robot_can_bang_pid_1.sce` bằng Scilab.

Kết nối Wi-Fi ESP32 và mở http://192.168.4.1/ui/r12?v=rt8-net5 . Đo lại gyro khi thân xe yên, xác nhận xe trên nền và bật cân bằng. Chờ sẵn sàng rồi giữ nút hướng; nhả để hãm và giữ cân bằng. Có thể trượt ngón tay giữa các nút để đổi hướng liên tục. Hướng chéo là đường cong; xe không đi ngang. Lùi trái/phải đặt tên theo quỹ đạo lùi.

Mặc định50cm/s, mức nhanh72cm/s là cấu hình thử cần kiểm chứng trên xe thật. Quay tại chỗ1,2/2,2rad/s (khoảng69/126° mỗi giây); đường cong0,80/0,55rad/s ở mức thường/nhanh trước khi áp dụng giới hạn theo tốc độ bánh, góc thân và khả năng bám. Giữ nút thì bám tốc độ quay liên tục, không đặt một góc tổng phải quay cho xong. Quay tại chỗ không bị nhận nhầm là kẹt do sai lệch vị trí tiến/lùi cũ. Khi thả, tốc độ quay đặt về0 ngay; xóa tích phân của lệnh quay và hãm bằng phản hồi có giới hạn, rồi giữ hướng tại điểm mới. Khi bị kẹt thật hoặc không đủ điều kiện cân bằng, vẫn hủy lệnh và yêu cầu nhả rồi nhấn lại.

Mỗi lần nhấn/đổi chiều lấy mốc vị trí hiện tại, bỏ điểm đặt của thao tác trước. Thả nút bỏ mục tiêu vị trí cũ và hãm tiến/lùi theo đường giảm tốc0,60m/s²; đảo chiều dùng0,45m/s² để tránh nghiêng mạnh. Chỉ chốt mốc giữ mới sau khi thân/bánh đủ yên; hãm chưa ổn có giới hạn1m/4giây. Xe cần thời gian và quãng đường hãm theo vận tốc thực; thả nút không khóa bánh hoặc tắt motor.

Web gửi ngay khi nhấn/nhả/đổi hướng, làm mới lệnh40ms, lấy trạng thái200ms và cập nhật bảng chi tiết500ms. Mục xác nhận STM32 đo thời gian từ gửi lệnh tới khi nhận dữ liệu xác nhận qua web, có cả chu kỳ hiển thị; không phải độ trễ một chiều hoặc cam kết thời gian thực cứng của Wi-Fi. Log40 cột,50Hz, khung122byte có số thứ tự/thời điểm STM32 nhận lệnh.

Giữ luật lực/observer R12, hiệu chuẩn và dựng xe60°. Khi tốc độ đang tiến gần trần0,8m/s và vượt điểm đặt, vào chế độ phục hồi có giới hạn trước khi chạm ngưỡng cắt. Sau khi mất lệnh và đã đứng ổn định, tiếp tục giữ cân bằng, chờ thao tác mới. Bảo vệ I2C, thời gian vòng lặp, heartbeat, góc, tốc độ, quãng trôi và phục hồi4giây vẫn có hiệu lực; đóng/ẩn/mất trang điều khiển làm hết lease di chuyển 350 ms để hãm rồi giữ cân bằng; gateway tiếp tục heartbeat khi thấy STM32 đang cân bằng. STOP và bảo vệ cứng vẫn dừng motor.

Chạy thử mức thường trước, rồi mức nhanh; thử tiến/lùi, quay tại chỗ, giữ cả bốn hướng chéo, nhả và đổi hướng. Dừng rồi chờ2giây để tải log. Bộ nhớ đầy không tự dừng xe nhưng không ghi thêm; cần tải và xóa khi dừng trước lượt tiếp theo. Tốc độ cao làm giảm dự trữ chịu va đập; không có bảo đảm phục hồi sau mọi cú va hoặc khi bánh trượt.

**Lịch sử RT4, không dùng làm thông số RT8:** Bản RT4 bổ sung sửa log `(26)`: thao tác mới bắt đầu từ vận tốc đo được, không kế thừa đường tốc độ cũ. Ngưỡng phục hồi góc khi có thao tác hợp lệ/đang hãm là12°, khi giữ yên vẫn8°; ngưỡng gyro60°/s, sai lệch tốc độ0,35m/s và bảo vệ thường không đổi. Quay tại chỗ giữ tâm và giảm tốc quay nếu tâm lệch; không nhận nhầm là kẹt tịnh tiến. Sau phục hồi, mốc giữ không trôi theo xe mãi. Web xử lý thêm sự kiện nhả trên iOS, báo đã nhả ngay và không lưu lệnh khi chưa sẵn sàng. Các sửa log24/25 vẫn giữ. Cửa mô phỏng là `robot_can_bang_pid_1.sce`. Chưa tự nạp xe hoặc bảo đảm dừng tức thời trong mọi điều kiện.

## Bàn giao source RT8 / NET5 — 30/10/2026

Source firmware và mô phỏng trong nhánh này được đồng bộ từ laptop, không đổi luật điều khiển trong lượt bàn giao. Tài liệu hiện hành: [HANDOFF](HANDOFF.md), [mô phỏng](Simulation/README.md), [Hardware KiCad](Hardware/README.md). Không có xác nhận thử RT8 mới trên xe. Log người dùng, đầu ra build và DLL không được đưa lên Git.
