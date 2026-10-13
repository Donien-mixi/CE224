# Xe hai bánh tự cân bằng — CE224

Đồ án STM32F411 + ESP32-S3. Nhánh `main` chứa mã nguồn và tài liệu để nhóm tự build, vận hành và phát triển tiếp; không cần log hoặc bản build trên máy người viết.

## Bắt đầu

1. **STM32:** mở STM32CubeIDE → File → Import → Existing Projects into Workspace → chọn `Xe_Can_Bang`. Build cấu hình Debug rồi nạp qua ST-Link. `Debug/` sẽ được IDE tạo lại; không dùng thư mục gốc repo làm workspace. Đã build với CubeIDE 2.1.1.
2. **ESP32:** mở `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` bằng Arduino IDE, cài ESP32 core và WebSockets. Cấu hình ESP32-S3 N16R8/OPI PSRAM theo [hướng dẫn gateway](ESP32_S3_Gateway/README.md). Đã build với ESP32 core 3.3.11 và WebSockets 2.7.2.
3. **Web:** kết nối Wi-Fi `TWIP_RACER_S3`, mật khẩu `12345678`, mở `http://192.168.4.1`. Firmware không tự chạy motor khi cấp nguồn. Chọn thử bánh khi kê cao, hoặc cân bằng khi bánh đã trên nền.
4. **Mô phỏng:** mở Scilab/SciNotes và Execute `robot_can_bang_pid_1.sce`. Xem [hướng dẫn mô phỏng](Simulation/README.md). Không cần Python, DLL hoặc dữ liệu test cũ.

## Mã nguồn và tài liệu

| Đường dẫn | Mục đích |
|---|---|
| `Xe_Can_Bang/Core` | Driver, lọc IMU, PID, FSM, giao tiếp và bài đo linh kiện |
| `Xe_Can_Bang/Drivers` | HAL/CMSIS kèm giấy phép; cần cho build STM32 |
| `Xe_Can_Bang/.project`, `.cproject`, `.ioc`, linker scripts | Import/build bằng CubeIDE và cấu hình CubeMX |
| `ESP32_S3_Gateway` | Gateway Wi-Fi/UART, web vận hành, đo linh kiện và tải log |
| `Simulation/model` | Phương trình cơ khí, cảm biến và điều khiển |
| `Simulation/config` | Bộ thông số và cấu hình lượt mô phỏng nhóm có thể chỉnh |
| `Simulation/scripts/run_team.sce` | Điểm chạy mô phỏng độc lập với dữ liệu cá nhân |
| `Simulation/tools` | Mã C tăng tốc tùy chọn và script biên dịch thư viện |
| `Docs/Phat_Trien_Tiep.md` | Trạng thái hiện hành, vị trí sửa code và giao thức |
| `Docs/Thong_So_Xe_Hien_Tai.md` | Tổng hợp thông số mô hình và những phần còn giả định |
| `Docs/Phan_Cung_Va_Ket_Noi.md`, `Huong_Dan_Di_Day_Toan_Bo_He_Thong.md` | Phần cứng, đấu nối |
| `Docs/Roadmap_Do_An.md`, `Docs/Thuat_Toan_Dieu_Khien.md` | Mục tiêu và nền tảng thuật toán |

## Trạng thái bàn giao

Mã firmware hiện hành là **BAL-20261012-R10S1**; web **TWIP-WEB-20261012-BAL-R10S1**. R10S1 giữ vòng cân bằng R10, thay điều kiện bắt đầu và lưu thêm dữ liệu trước ARM. R10 từng có lượt tự cân bằng khoảng165 giây, nhưng vẫn có trôi và quay; R10S1 chưa được xác nhận tốt hơn trên xe. Đây chưa phải bản nghiệm thu tự cân bằng ổn định.

PID nền `[48, 3.6, 32, 0.5, 0.3]`; giá trị firmware chính thức nằm trong `Xe_Can_Bang/Core/Inc/balance_profile.h`. Mô phỏng không tự sửa hoặc nạp firmware. Nhóm có thể thay phương pháp điều khiển, hiệu chuẩn và điều kiện thử theo mục tiêu của mình; không bắt buộc tiếp tục quy trình tuning trước đây.

## Dữ liệu không đưa lên Git

Không commit workspace `.metadata`, thư mục build `Debug/Release/.build`, binary, log xe, ảnh/báo cáo/kết quả mô phỏng và lịch sử thử nghiệm. Các file này được giữ tại máy thực nghiệm; lần chạy mới tự sinh lại đầu ra. `.gitignore` đã khai báo các nhóm đó.

Các tài liệu thiết kế gốc có một số con số/đoạn mô tả từ phiên bản trước. Khi đối chiếu cách firmware hiện hành hoạt động, dùng mã nguồn và `Docs/Phat_Trien_Tiep.md`; không xem các ví dụ PID trong tài liệu thuật toán là thông số đang nạp.
