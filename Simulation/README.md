> **Trạng thái đồ án · 15/11/2026.** Nền R12 đã được thử cân bằng và dựng từ 60°. Giai đoạn điều khiển web đã có 8 hướng và log. Workspace đang ở **RT29-NOCUT / NET30**, đã build, kiểm thử trên máy và push Git. Mô hình tính pin kê thêm 10 mm. PCB local một mặt đồng đã qua kiểm tra KiCad với 31 dây cầu. Xem [tổng quan và các việc còn mở](../Docs/Tong_Quan_Do_An.md).
>
> **Mô phỏng dùng chung header firmware hiện hành**, nên chạy lại là kiểm luôn bản đang nạp. Lần
> chạy 15/11/2026 với RT29: **12 ca tốc độ + 12 ca biên + 12 ca khởi hành 50 cm/s, tất cả 20/20 s,
> không fault** (`node tools/run_governor_sim.cjs --stress` rồi thêm `--edge`, và
> `--stress --launch --launch50`). Thư mục `results/` chỉ là đầu ra, không đưa lên Git.

# Mô phỏng R12 hiện hành — RT19 HOLD80

RT19 giữ nguyên governor RT17/RT18, chỉ sửa mốc hướng sau quay (tối đa0,6s rồi đóng băng) và chốt lại mốc vị trí200ms sau khi phanh yên; preset cao nhất là80cm/s.  Chạy `node tools/run_governor_sim.cjs --stress --launch --launch50` để thử 12ca khởi hành50 với mốc±2/±3/±4°. `--launch` yêu cầu kèm `--stress` (12ca) vì vector mốc có12 phần tử. Có thể đặt `TWIP_CASE_IDS=3,6` để đối chiếu ca cụ thể; bỏ biến khi chạy đủ12ca. `--launch` còn kiểm tra tham chiếu đã thực hiện đúng hướng, không tính xe đứng im không nhận lệnh là đạt.

Nền RT17 dùng cùng điểm vào ở dưới. Model hiện giữ đúng thao tác đang giữ trong REC1 (không tự giả định nhả nút), cập nhật20ms, có phanh chung khi REC1 vào, giới hạn lệnh phục hồi và mốc predictor từ đứng yên. Header được chụp trước biên dịch; thay preset không reset governor mỗi chu kỳ. 

`node tools/run_governor_sim.cjs --stress --disturbance` chạy bộ thử thách1–4N×80ms; có ca không đạt và lệnh trả lỗi khi có ca ngã/cắt bảo vệ. Không xem status=completed của script là mọi tiêu chuẩn đều đạt. Có thể đặt `TWIP_PUSH_SCALE` để khảo sát biên độ khác, nhưng phải công bố hệ số khi trình bày kết quả. `TWIP_HEADER_DIR` cho phép đối chiếu snapshot header, `TWIP_GOVERNOR_HEADER` chỉ thay governor. Mỗi lượt lưu trace `.governor.csv` cho gia tốc/duty/mốc predictor/cờ hạn chế.

Điểm vào hiện hành là `robot_can_bang_pid_1.sce` ở gốc repo. Trên máy mới, trước hết chạy `./Simulation/build_native.ps1 -Compiler C:/msys64/ucrt64/bin/gcc.exe` để có thư viện tích phân/tiếp xúc. Sau đó Execute launcher trong Scilab có giao diện: nó biên dịch wrapper từ header STM32 hiện tại, gồm governor và điểm đặt từ tick, chạy 12 ca tốc độ rồi vẽ biểu đồ. Cần Node trên PATH, GCC tại `C:/msys64/ucrt64/bin/gcc.exe` (đường dẫn trong `tools/run_governor_sim.cjs`). Kết quả ở `Simulation/results/runs/`. Launcher kiểm tra đủ thời gian và guard trước khi vẽ; lỗi mô phỏng được báo, không coi là đạt.

Chạy tự động: `node tools/run_governor_sim.cjs --stress`. Thêm `--edge` để chạy 12 ca đứng yên/lệch mốc góc/trọng tâm/quay hai chiều/bấm–nhả nhanh. Không nạp firmware. Kết quả mới bật `supervisor_count_position` để đồng nhất vị trí giám sát với vị trí encoder trong luật lực R12. Không ghi đè kết quả cũ.

Các số liệu lịch sử của bộ 24/36 ca nằm trong git history (báo cáo RT16 đã gỡ khỏi cây làm việc). 24 ca đủ 20 s không có lỗi bảo vệ chưa chứng minh chạy vô hạn hoặc mọi va chạm. Mô hình vẫn có giả định lực có tải/ma sát; một số ca MAX95 chỉ giữ khoảng80cm/s.

Phần dưới là hướng dẫn và lịch sử bộ tổng quan RT8. `current/review.sce` / `review_cli.sce` cũ không tích hợp đầy đủ governor hiện hành; không dùng chúng để kết luận đạt RT16/MAX95. `build_native.ps1` vẫn phục vụ các bộ chạy cũ đó.

## Lịch sử tổng quan RT8

Yêu cầu: Windows 64 bit, Scilab 2026.1 và GCC/MinGW 64 bit. Không cần log riêng, báo cáo hay thư viện nhị phân từ máy người phát triển.

1. Từ thư mục gốc repo, chạy PowerShell: `./Simulation/build_native.ps1`. Nếu GCC chưa có trên PATH, thêm `-Compiler C:/duong/dan/gcc.exe`.
2. Mở Scilab, Execute file `robot_can_bang_pid_1.sce` ở gốc repo.
3. Xem console và các biểu đồ góc, tốc độ, quãng đường. Kết quả tự lưu trong `Simulation/results/runs/`, được bỏ qua bởi Git.

Chạy không biểu đồ: Execute `Simulation/current/review_cli.sce` bằng Scilab CLI; script này thoát Scilab sau khi xong.

`current/` chứa bộ chạy hiện hành: 12 trường hợp cân bằng, tiến 50/72 cm/s, lùi, đảo chiều, nhả lệnh trước vật cản giả định, quay trái/phải tại chỗ và bốn hướng chéo. Biểu đồ thêm góc quay thực/đặt và tốc độ quay đặt. `model/` chứa hình học/động lực học; `config/vehicle_baseline.sce` là thông số xe, `motor_supply.sce` là nguồn, `motor_pair_r8_20261011.sce` là đặc tính từng bánh đã hiệu chuẩn. `build_native.ps1` biên dịch các phần điều khiển trực tiếp từ header firmware STM32 để tránh bản sao khác nhau.

Giữ nguyên kích thước và bố trí xe. Tải, ma sát, độ trượt và va chạm chưa được nhận dạng đầy đủ. Các thử nghiệm này không chứng minh khả năng phục hồi ở mọi lực xô hoặc góc lớn; khởi động 60° đã được xác nhận trên xe nhưng mô hình tiếp xúc ở góc lớn còn hạn chế. Không tự động sửa hay nạp firmware khi chạy mô phỏng.

`scripts/run_team.sce` là bộ đối chiếu PID cũ, không phải điểm vào R12 hiện hành.

Mô phỏng dùng đúng header firmware, gồm tâm quay cố định, tham chiếu vận tốc mới khi nhấn, hãm tiến/lùi và phục hồi phân biệt thao tác chủ động. Cần chạy lại `build_native.ps1` sau khi cập nhật; tên alias `motion_rt5.dll` / `recovery_rt5.dll` là di sản. DLL vừa build sử dụng header STM32 RT8 hiện có, không phải bản firmware RT5.

Lịch sử RT5 (RT8 vẫn giữ cơ chế này, nhưng gyro trigger hiện là 150°/s): xác nhận 60 ms với sai lệch góc/tốc độ trong thao tác hợp lệ; gyro vẫn tác động ngay. `current/speed_limits_cli.sce` chuẩn bị kiểm tra yêu cầu 0,72–0,90 m/s; lượt 23/10 chưa chạy được do JVM thiếu bộ nhớ, không dùng để mở tốc độ firmware. 

## Phạm vi dữ liệu và kiểm tra bàn giao

Bộ 12 bài tổng quan dùng snapshot `config/vehicle_baseline.sce`, không cần log riêng của tác giả. `config/load_measurement_20261010.sce` là đường nạp chiến dịch đo cũ, chỉ dùng khi có bộ CSV đo tương ứng dưới `data/processed/`; **dữ liệu đó không có trong Git** (đã chuyển ra ngoài repo khi dọn dẹp 15/11/2026). Tệp duy nhất trong `data/` được giữ là `data/raw/twip_balance_R12_27_replay.csv` vì `tools/test_rt8_replay.cjs` cần. Các CSV `pid_candidates_*` là cấu hình đối chiếu lịch sử, không chọn lại luật PID cho firmware R12.

Ngày 30/10/2026 đã biên dịch lại các thư viện native với GCC 64 bit. Chưa chạy lại toàn bộ 12 quỹ đạo trong lượt push này; build thành công không chứng minh an toàn trên xe.
