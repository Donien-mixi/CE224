# PCB 160 × 65 mm — lỗ M3 theo tọa độ người dùng

## Hệ tọa độ

Đơn vị người dùng là **cm**. Gốc ở giao hai cạnh bo gần H3, không phải tâm lỗ H3. X hướng về H4 (sang trái trên bản vẽ KiCad), dài 16 cm. Y hướng về H2 (lên trên), dài 6,5 cm. Nhìn từ mặt lắp linh kiện.

Góc trên trái bo trong KiCad: (50;50) mm; dưới phải: (210;115) mm. Quy đổi: **KiCad X = 210 − 10×X người dùng; KiCad Y = 115 − 10×Y người dùng**.

| Lỗ | Vị trí KiCad | X;Y người dùng (cm) | X;Y KiCad (mm) |
|---|---|---|---|
| H1 | Trên trái | 15,25 ; 6,25 | 57,5 ; 52,5 |
| H2 | Trên phải | 0,75 ; 6,25 | 202,5 ; 52,5 |
| H3 | Dưới phải | 0,75 ; 0,25 | 202,5 ; 112,5 |
| H4 | Dưới trái | 15,25 ; 0,25 | 57,5 ; 112,5 |

Các tâm này thay thế vị trí tạm cách mép 4,5 mm. Khoảng tâm ngang **145 mm**, dọc **60 mm**. Tâm cách cạnh ngắn 7,5 mm, cạnh dài 2,5 mm.

## Lỗ và khoảng gá

- Lỗ M3 **Ø3,2 mm NPTH**, không nối điện; chỉ trên PCB, schematic điện không đổi.
- Vùng tránh đồng Ø8 mm trên cả hai mặt đã di chuyển theo từng lỗ. Cấm track/via/đổ đồng trong vùng này, kết hợp clearance 2,4 mm từ mép NPTH đến pad đồng; không cấm chính lỗ NPTH.
- Mép lỗ cách mép bo gần nhất chỉ **0,9 mm**. Đã giữ nguyên tọa độ được cung cấp, cần kiểm độ bền khi khoan/siết trên vật liệu bo thực.
- Đầu vít Ø6,4 mm có thể nhô **0,7 mm** qua mép dài; vòng đệm Ø8 mm nhô **1,5 mm**. Đối chiếu vít/vòng đệm/cọc đỡ với khung trước khi chế tạo. DRC không xác nhận độ bền hoặc khả năng gá cơ khí.
- H2 gần anten ESP32; vùng cấm đồng vẫn kéo tới mép bo. Kiểm vật liệu/khoảng hở bộ vít và hiệu quả Wi-Fi; lỗ không mạ không đảm bảo vít kim loại không ảnh hưởng RF.

## Hướng xe đã xác nhận

- **Phía trước / tiến:** phía hàng **7 chân BMI160**, hướng xuống bản vẽ KiCad, chiều giảm Y người dùng.
- **Phía sau / lùi:** phía hàng **5 chân BMI160**, hướng lên bản vẽ, chiều tăng Y người dùng.
- Đây là quy ước cơ khí, không tự đổi trục IMU, dấu pitch, dấu motor hoặc firmware.

Đã dịch lỗ/vùng tránh đồng, dịch C7 sang trái 1 mm để tránh H3, chỉnh dây vào C7 và đổ lại GND. Năm module, net điện, giá trị linh kiện và các đường UART giữ nguyên. Thêm mũi tên trước/sau trên lớp bản vẽ, không thêm đồng/lỗ phụ.

DRC=0, parity=0, unconnected=0; các net pad điện khớp trước khi dời lỗ. Tọa độ tâm đã có số đo người dùng; bộ gá và các vấn đề cơ khí/RF khác vẫn cần xác nhận. Xem MOUNTING_HOLES.json và CHECK_drc_final.json.

Cập nhật khoảng hở H4: BZ1 dịch thêm 2 mm sang phải trong KiCad, tâm mới (64;107) mm. Giữ nguyên bốn tâm M3; chỉnh dây và đổ lại GND. DRC/parity/unconnected=0. Với đường bao BZ1 hiện tại, đầu vít Ø6,4 mm còn khoảng 1,75 mm; vòng đệm Ø8 mm còn khoảng 0,95 mm tới góc gần nhất.
