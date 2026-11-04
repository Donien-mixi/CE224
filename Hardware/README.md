# Thiết kế PCB TWIP

Mở bằng **KiCad 10.0 hoặc mới hơn**.

1. Chuyển sang nhánh `pcb`, tải/clone repo và giữ nguyên thư mục `Hardware`.
2. Mở `TWIP_Mainboard/TWIP_Mainboard.kicad_pro`.
3. Mở `.kicad_sch` trong Schematic Editor và `.kicad_pcb` trong PCB Editor.

Thư viện symbol `TWIP_Modules.kicad_sym`, thư viện footprint `TWIP_Carrier.pretty`, `sym-lib-table` và `fp-lib-table` đi kèm project. Đường dẫn thư viện dùng `${KIPRJMOD}` nên không cần đường dẫn máy tác giả. Giữ các thư viện này cùng project.

Bản PCB hiện tại có khung 160 × 65 mm, lắp các module STM32F411 Black Pill, ESP32-S3, A4950, BMI160 và XL4015. Thư viện gồm cả footprint cầu nối dây của bản một lớp.

Nhánh này chỉ cập nhật file thiết kế KiCad và các thư viện cần dùng. Không kèm báo cáo AI, log, ảnh xem trước, file in, bản lưu cũ hay file tạm. Firmware và mô phỏng kế thừa từ `main` tại lúc tạo nhánh, không phải bản firmware mới trên máy tác giả.
