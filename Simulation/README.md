# Mô phỏng tổng quan R12 8DIR RT1

Yêu cầu: Windows 64 bit, Scilab 2026.1 và GCC/MinGW 64 bit. Không cần log riêng, báo cáo hay thư viện nhị phân từ máy người phát triển.

1. Từ thư mục gốc repo, chạy PowerShell: `./Simulation/build_native.ps1`. Nếu GCC chưa có trên PATH, thêm `-Compiler C:/duong/dan/gcc.exe`.
2. Mở Scilab, Execute file `robot_can_bang_pid_1.sce` ở gốc repo.
3. Xem console và các biểu đồ góc, tốc độ, quãng đường. Kết quả tự lưu trong `Simulation/results/runs/`, được bỏ qua bởi Git.

Chạy không biểu đồ: Execute `Simulation/current/review_cli.sce` bằng Scilab CLI; script này thoát Scilab sau khi xong.

`current/` chứa bộ chạy hiện hành: 12 trường hợp cân bằng, tiến 50/72 cm/s, lùi, đảo chiều, nhả lệnh trước vật cản giả định, quay trái/phải tại chỗ và bốn hướng chéo. Biểu đồ thêm góc quay thực/đặt và tốc độ quay đặt. `model/` chứa hình học/động lực học; `config/vehicle_baseline.sce` là thông số xe, `motor_supply.sce` là nguồn, `motor_pair_r8_20261011.sce` là đặc tính từng bánh đã hiệu chuẩn. `build_native.ps1` biên dịch các phần điều khiển trực tiếp từ header firmware STM32 để tránh bản sao khác nhau.

Giữ nguyên kích thước và bố trí xe. Tải, ma sát, độ trượt và va chạm chưa được nhận dạng đầy đủ. Các thử nghiệm này không chứng minh khả năng phục hồi ở mọi lực xô hoặc góc lớn; khởi động 60° đã được xác nhận trên xe nhưng mô hình tiếp xúc ở góc lớn còn hạn chế. Không tự động sửa hay nạp firmware khi chạy mô phỏng.

`scripts/run_team.sce` là bộ đối chiếu PID cũ, không phải điểm vào R12 hiện hành.
