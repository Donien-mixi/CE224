# R12 8DIR RT1 — kiểm chứng điều khiển 8 hướng

Bản ngày 2026-10-19 dùng trực tiếp các header điều khiển C của firmware trong mô hình Scilab phi tuyến. Không thay đổi kích thước, khối lượng hay tăng lực motor để làm bài thử đạt.

## Cấu hình được chọn

| Đại lượng | Thường | Nhanh |
|---|---:|---:|
| Tốc độ tiến/lùi | 0,50 m/s | 0,72 m/s |
| Quay tại chỗ | 1,20 rad/s | 2,20 rad/s |
| Quẹo khi tiến/lùi | 0,80 rad/s | 0,55 rad/s |

Đây là giá trị tham chiếu. Bộ giới hạn giảm tốc độ quẹo khi thiếu dự trữ tốc độ bánh, nghiêng nhiều hoặc góc thực chưa theo kịp. Tham chiếu hướng tích lũy liên tục khi giữ nút; không giới hạn tổng số vòng quay. Tăng tốc tuyến tính 0,30 m/s² và giảm còn 0,15 m/s² trên 0,50 m/s; giảm tốc 0,45 m/s².

## Chạy lại

Cần Python có NumPy, GCC 64 bit và Scilab 2026.1 64 bit. Chạy từ thư mục gốc repo; thay đường dẫn trình biên dịch/Scilab theo máy.

```powershell
python Simulation/experiments/drive8_realtime_20261019/prepare_release.py
powershell -File .build/rt1_release_check/Simulation/build_native.ps1 -Compiler C:/msys64/ucrt64/bin/gcc.exe
& 'C:/Program Files/scilab-2026.1.0/bin/WScilex-cli.exe' -nb -f "$PWD/.build/rt1_release_check/Simulation/current/qualify.sce"
python Simulation/experiments/drive8_realtime_20261019/analyze_release.py
python Simulation/experiments/drive8_realtime_20261019/plot_release.py
& 'C:/Program Files/scilab-2026.1.0/bin/WScilex-cli.exe' -nb -f "$PWD/.build/plot_rt1_release.sce"
```

Đóng tiến trình Scilab đang giữ DLL trước khi biên dịch lại. Script tạo bản sao kiểm chứng trong `.build/rt1_release_check`; không tự sửa firmware. Kết quả và ảnh nằm trong `Simulation/reports/drive8_realtime_20261019` và được bỏ qua bởi Git.

## Kết quả và phạm vi

Lượt cuối đạt 76/76: 48 ca tám hướng ở hai tốc độ, điện áp 11,7/11,2 V và bất đối xứng motor; 16 ca đổi hướng; 4 ca mất luồng lệnh nhưng vẫn còn heartbeat; 4 ca chạy 60 giây; 4 ca lực nhỏ ±0,2 N trong 0,1 giây khi quẹo. Kiểm tra chạy đủ thời gian, cân bằng cuối lượt, dừng sau hủy lệnh và quay liên tục. Các mức 0,75–0,80 m/s đã thử nhưng chưa đạt yêu cầu nên không chọn.

Hai lỗi phối hợp được sửa: hủy lệnh đã dừng ổn định không còn gây cắt motor trễ; khi chạy nhanh, dự trữ phục hồi được kích hoạt sớm trước ngưỡng quá tốc thường. Không tự chạy lại lệnh cũ sau phục hồi. Luật cân bằng R12, bộ lọc và dựng xe 60° được giữ.

Mô hình chưa xác nhận độ bám/lật ngang, va chạm cứng hoặc độ trễ Wi-Fi thực. Kết quả này không chứng minh xe thực chịu được mọi lực đẩy. Cần nạp cả STM32 và ESP32 vì khung UART RT1 đổi sang 122 byte; thử tốc độ thường trước, sau đó nhanh, và thu log thực để đối chiếu. Chu kỳ gửi web 40 ms và đọc trạng thái 100 ms không phải bảo đảm thời gian thực cứng.
