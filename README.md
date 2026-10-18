# Xe hai bánh tự cân bằng — R12 SPEED2

Mã nguồn đang dùng: STM32 `BAL-20261018-R12-SPEED2`, ESP32 `TWIP-WEB-20261018-BAL-R12-SPEED2`.

- `Xe_Can_Bang/`: mở project trong STM32CubeIDE, Build rồi nạp bằng ST-LINK.
- `ESP32_S3_Gateway/`: mở sketch cùng tên trong Arduino IDE; xem README trong thư mục để chọn board.
- `robot_can_bang_pid_1.sce`: điểm vào mô phỏng tổng quan; xem `Simulation/README.md` trước lần chạy đầu.

Nạp cả hai board để đồng bộ giao thức. Kết nối Wi-Fi của ESP32 rồi mở http://192.168.4.1/ui/r12 . Đo lại gyro khi xe đứng yên, chọn chế độ xe trên nền và bật cân bằng. Giữ TIẾN/LÙI để chạy, nhả để giảm tốc. Mặc định 40 cm/s; mức 60 cm/s cần tiếp tục kiểm chứng thực tế. Khởi động nghiêng đến 60° giữ nền R12 đã thử trên xe. Sau va chạm/phục hồi, xe hủy mục tiêu di chuyển cũ và yêu cầu lần nhấn mới; lỗi nghiêm trọng cần bật cân bằng lại.

Kết quả chạy, log, báo cáo nghiên cứu và file build không thuộc bộ mã nguồn bàn giao. Mô phỏng dùng các giả định tải/ma sát, không bảo đảm phục hồi sau mọi va chạm trên xe thật.
