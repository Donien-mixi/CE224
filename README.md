# Xe hai bánh tự cân bằng — R12 8DIR RT1

STM32 `BAL-20261019-R12-8DIR-RT1`; ESP32 `TWIP-WEB-20261019-BAL-R12-8DIR-RT1`. Nạp cả hai board vì giao thức dữ liệu đã đổi.

- Mở project `Xe_Can_Bang/` trong STM32CubeIDE, Build rồi nạp ST-LINK.
- Mở `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` trong Arduino IDE; cấu hình board trong README cùng thư mục.
- Mô phỏng: xem `Simulation/README.md`, sau đó Execute `robot_can_bang_pid_1.sce` bằng Scilab.

Kết nối Wi-Fi ESP32 và mở http://192.168.4.1/ui/r12?v=rt1 . Đo lại gyro khi thân xe yên, xác nhận xe trên nền và bật cân bằng. Chờ sẵn sàng rồi giữ nút hướng; nhả để hãm và giữ cân bằng. Có thể trượt ngón tay giữa các nút để đổi hướng liên tục. Hướng chéo là đường cong; xe không đi ngang. Lùi trái/phải đặt tên theo quỹ đạo lùi.

Mặc định50cm/s, mức nhanh72cm/s là cấu hình thử cần kiểm chứng trên xe thật. Quay tại chỗ1,2/2,2rad/s (khoảng69/126° mỗi giây); đường cong0,80/0,55rad/s ở mức thường/nhanh trước khi áp dụng giới hạn theo tốc độ bánh, góc thân và khả năng bám. Giữ lệnh thì điểm đặt quay tiếp tục tăng, không giới hạn góc tổng. Khi không bám kịp, tự giảm tốc quay; nếu bị kẹt thật sẽ hủy lệnh và yêu cầu nhả rồi nhấn lại.

Web gửi ngay khi nhấn/nhả/đổi hướng, làm mới lệnh40ms và hiển thị100ms. Mục xác nhận STM32 đo thời gian từ gửi lệnh tới khi nhận dữ liệu xác nhận qua web, có cả chu kỳ hiển thị; không phải độ trễ một chiều hoặc cam kết thời gian thực cứng của Wi-Fi. Log40 cột,50Hz, khung122byte có số thứ tự/thời điểm STM32 nhận lệnh.

Giữ luật lực/observer R12, hiệu chuẩn và dựng xe60°. Khi tốc độ đang tiến gần trần0,8m/s và vượt điểm đặt, vào chế độ phục hồi có giới hạn trước khi chạm ngưỡng cắt. Sau khi mất lệnh và đã đứng ổn định, tiếp tục giữ cân bằng, chờ thao tác mới. Bảo vệ I2C, thời gian vòng lặp, heartbeat, góc, tốc độ, quãng trôi và phục hồi4giây vẫn có hiệu lực; rời tab/khóa màn hình/mất kết nối trang điều khiển vẫn có thể dừng motor.

Chạy thử mức thường trước, rồi mức nhanh; thử tiến/lùi, quay tại chỗ, giữ cả bốn hướng chéo, nhả và đổi hướng. Dừng rồi chờ2giây để tải log. Bộ nhớ đầy không tự dừng xe nhưng không ghi thêm; cần tải và xóa khi dừng trước lượt tiếp theo. Tốc độ cao làm giảm dự trữ chịu va đập; không có bảo đảm phục hồi sau mọi cú va hoặc khi bánh trượt.
