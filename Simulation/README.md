# Mô phỏng dành cho nhóm

Bộ mã này chạy độc lập với các log, báo cáo và archive thử nghiệm trên máy cá nhân. Đã dùng Scilab2026.1.0. Không cần Python hay thư viện C biên dịch sẵn.

## Chạy

Mở Scilab/SciNotes, mở file gốc `robot_can_bang_pid_1.sce`, chọn Execute → File with no echo; hoặc:

```scilab
exec("<thu_muc_repo>/robot_can_bang_pid_1.sce", -1);
```

Điểm vào là `scripts/run_team.sce`. Mặc định chạy một ca20giây với vòng điều khiển R10, xuất đồ thị góc/mục tiêu, quãng encoder và PWM trái/phải. Không tự tìm PID hoặc sửa firmware.

Kết quả mới ở `results/runs/<thoi_gian>_team_baseline/`: console, trajectory/summary CSV, tên cột và snapshot đầu vào. Git bỏ qua toàn bộ kết quả này.

## Chỉnh sửa

- `config/team_run.sce`: PID, thời lượng, góc/tốc độ ban đầu, giả định lực/ma sát/trễ, seed và số phân bước.
- `config/vehicle_baseline.sce`: snapshot hệ số encoder, đường tốc độ motor không tải, hiệu chuẩn và nhiễu. Là thông số cấu hình đã tổng hợp, không chứa log từng mẫu.
- `model/robot_can_bang_pid_1.sce`: bố trí linh kiện, tính trọng tâm/quán tính, phương trình cơ khí và cảm biến.
- `model/capture_authority.sci`: tăng phản hồi bắt lại của R10.
- `config/startup_r10s1.sce`: mô tả điều kiện cho phép ARM; runner này bắt đầu từ trạng thái đã thả tay, không mô phỏng tay giữ xe hoặc tự kiểm chứng lịch sử READY300ms.

Nhiễu của entry point nhóm được xấp xỉ Gaussian từ mức nhiễu tĩnh đã tổng hợp; không phát lại chuỗi nhiễu thực. Do đó không hứa lặp đúng đồ thị của các lần test cá nhân trước đây.

## Giới hạn cần biết

Xe888g, bánh65mm, trọng tâm tính từ phân bố khối lượng. Lực/ma sát/đáp ứng mô-men dưới tải vẫn là giả định. Runner mặc định dùng mô hình pitch phẳng và lực trung bình, **chưa kiểm chứng quay/roll/trượt**. Các ngưỡng ngắt giữ25°/30cm/0,8m/s; mô phỏng chạy hết20giây không phải xác nhận xe thật sẽ cân bằng tốt.

Mô hình hai bánh `model/pair_coupled.sci` và mã C `tools/twip_pair_integrate.c` được giữ để nhóm phát triển tiếp. Đây là phần chẩn đoán có điều kiện, không tự bật trong runner mặc định; cần biên dịch thư viện và lập kịch bản riêng trước khi sử dụng.

## Tăng tốc tùy chọn trên Windows

Mặc định có thể chạy hoàn toàn bằng Scilab, nhưng chậm hơn. Nếu có GCC64bit phù hợp với Scilab64bit:

```powershell
./Simulation/tools/build_native_kernel.ps1 -Compiler gcc
```

Có thể truyền đường dẫn compiler trên máy mình. Đóng phiên Scilab đang nạp DLL trước khi biên dịch lại. Không bật fast-math. DLL và metadata build được sinh tại `tools/`, Git bỏ qua. Nếu DLL không có hoặc nạp lỗi, mô hình chính dùng phương trình Scilab thay thế.

Nhóm có thể thêm cách tìm PID, bài thử hay phương pháp điều khiển riêng. Giữ các thông số đã đo tách khỏi giả định, kiểm tra bước tính và đối chiếu xe thật trước khi đổi firmware.
