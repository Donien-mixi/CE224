> **Trạng thái:** bản hiện hành **RT29-NOCUT / NET30** — xem [tổng quan](Tong_Quan_Do_An.md) và [HANDOFF](../HANDOFF.md).

> Các mô tả theo phiên bản cũ bên dưới là tài liệu đối chiếu. Dùng tổng quan phía trên để xác định cấu hình hiện hành.

> **Hiện tại: R12 SPEED2** — chọn 40/60 cm/s; bỏ mục tiêu cũ sau phục hồi/kẹt, chờ lần giữ mới; va chạm mạnh có thể dừng motor. [Nạp và thử](../Docs/Van_Hanh_R12_SPEED2.md). Mức nhanh60 cần xác nhận trên xe thật.

> **Hiện tại: R12 SPEED1** — tiến/lùi liên tục ở0,40m/s, gấp4lần MOVE1; bỏ chặn quãng đường60cm. Giữ luật cân bằng R12. [Hướng dẫn nạp và thử](../Docs/Van_Hanh_R12_SPEED1.md). Đây là mức tăng tốc cần kiểm chứng trên xe, chưa bảo đảm mọi điều kiện đua.

> **Giai đoạn2 hiện tại: R12 MOVE1** — giai đoạn1 đã được người vận hành xác nhận; thêm giữ nút TIẾN/LÙI, nhả để giảm tốc và cân bằng tại điểm mới. [Nạp và vận hành](Van_Hanh_R12_MOVE1_Tien_Lui.md). Các phần BASE60/REC1 bên dưới là nền đối chiếu; firmware hiện tại là `BAL-20261018-R12-MOVE1` / `TWIP-WEB-20261018-BAL-R12-MOVE1`.

> **Bản thử bổ sung hiện tại: R12 REC1** — giữ luật cân bằng/chống xoay và dựng60° của BASE60, thêm phục hồi sau đẩy có giới hạn. [Cách nạp và thử](Thu_R12_REC1_Phu_Hoi_Sau_Day.md). Mức1,5N/0,1s mới được kiểm tra trên mô hình; chưa đo xác nhận lực trên xe thật.

# Phát triển tiếp trên R12 BASE60

Cấu hình duy nhất hiện hành là R12 khởi động đến ±60°. Người vận hành đã thử60° thành công và chọn S60X làm nền tảng. BASE60 bỏ lựa chọn R7 khỏi web và luồng cân bằng, giữ nguyên luật điều khiển, bộ ước lượng và pha dựng S60X.

STM32: `BAL-20261018-R12-REC1`. ESP32: `TWIP-WEB-20261018-BAL-R12-REC1`. Xem [nạp và vận hành](Van_Hanh_R12_60.md), [thông số xe](Thong_So_Xe_Hien_Tai.md).

| Nội dung | Nguồn |
|---|---|
| Phản hồi trạng thái, bộ ước lượng, phân bổ lực, giữ hướng | `Xe_Can_Bang/Core/Inc/phase1_catalog_control.h` |
| Gain/matrix/tham số R12 đóng băng | `Xe_Can_Bang/Core/Inc/phase1_catalog_params.h` |
| Hiệu chuẩn/ARM/ngưỡng/build ID | `Xe_Can_Bang/Core/Inc/balance_profile.h` |
| Phục hồi sau đẩy có giới hạn | `Xe_Can_Bang/Core/Inc/balance_recovery.h` |
| Điểm đặt di chuyển / luật bám điểm đặt | `Core/Inc/balance_motion.h`, `phase2_motion_control.h` |
| Pha dựng theo góc lúc bật | `Xe_Can_Bang/Core/Inc/balance_startup.h` |
| Vòng200Hz, ARM/STOP và telemetry | `Xe_Can_Bang/Core/Src/robot_fsm.c` |
| IMU, bộ lọc góc, encoder | `Core/Src/bmi160.c`, `filter.c`, `encoder.c` |
| Nhận lệnh từ gateway | `Core/Src/esp32_comm.c` |
| Web vận hành, trạng thái và tải log | `ESP32_S3_Gateway/dashboard_html.h`, `balance_web.h` |
| Chủ điều khiển, heartbeat và nguồn STOP | `ESP32_S3_Gateway/control_session.h` |
| Bài đo motor/encoder/IMU | `wheel_measurement.h`, `wheel_tests_html.h`, `measurement_web.h` |
| Cửa vào mô phỏng R12 | `robot_can_bang_pid_1.sce` → `Simulation/scripts/run_r12_baseline.sce` |

Các đường dẫn `Core/...` trong bảng thuộc project `Xe_Can_Bang`. Mã PID cũ còn lại phục vụ tương thích thư viện/chẩn đoán; sửa `pid.c` không tuning được R12. `$PROFILE` bị bỏ qua; `$ARM,7` bị từ chối. Lệnh hiện hành gồm `$ARM,12`, `$STOP`, `$PING`, `$PULSE`, `$CALIB` và các lệnh đo. Giai đoạn2 dùng `$MOVE,direction,epoch,sequence` với nút giữ và lease350ms; `$CMD`/steer cũ vẫn bị khóa, chưa có rẽ.

Giao thức MOVE1: frame104byte, sync0x5ea5, CRC16; controller luôn12. Giữ đồng nhất `balance_wire.h` hai board. Log CSV V6 có35cột, control_phase=0thường/1dựng/2phục hồi, ghi100Hz khi hoạt động và2giây sau dừng, không ghi đè khi đầy. `control_v`/`control_theta` là ước lượng R12, `control_integral` là tích phân hướng Nm.

Trước khi thay bản nền: chạy `tools/test_r12_controller.cjs`, `test_r12_dashboard.cjs`, `test_r12_gateway.cjs`, `test_hardware_boot.cjs`, `test_wheel_web.cjs` bằng Node.js. `tools/build_balance_firmware.ps1` biên dịch hai board và ghi manifest; `tools/verify_balance_release.py` kiểm tra nguồn/file nạp/trang nhúng. Không tự nạp hoặc push Git.

Mô hình vật lý góc lớn chưa được kiểm chứng đủ; không thay giả định lực/ma sát để ép ca60° đạt. Giữ riêng kết quả mô phỏng, quan sát người vận hành và dữ liệu log. R7 chỉ còn trong lịch sử đối chiếu, không phải nền tảng phát triển tiếp. Bản build/log/kết quả/báo cáo tự sinh không đưa lên Git.
