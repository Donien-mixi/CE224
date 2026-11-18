> **Trạng thái:** bản hiện hành **STM32 RT38-CONT + ESP32 NET38** (18/11/2026) — xem [HANDOFF](../../HANDOFF.md).

> CẬP NHẬT03/11: motor trái đỏJ2.1→AOUT2, trắngJ2.6→AOUT1. W1/W2, schematic và danh sáchnet đã đổi đúng cực; kích thước giữ nguyên. Mô tả net không đổi bên dưới là so sánh ở thời điểm chuyển lớp trước sửa cực.

# PCB một mặt đồng — 02/11/2026

## Bản hiện hành · 06/11/2026

W6 được thay bằng đường đồng 3,3 V. W25 được thay bằng vùng đồng GND đã đổ lại. Vị trí cầu W còn lại giữ nguyên.

Carrier giữ160×65mm. Tâm thân BMI160 tại (130,00; 81,65) mm, cách tâm bo 0,85 mm. Gốc footprint U5 tại (130,40; 81,65) mm. Giữ hướng lắp U5, kích thước, lưới chân và net. Các module khác, đường bao và 4 lỗ M3 giữ nguyên.

Chỉ gia công đồng trên **B.Cu (mặt dưới/mặt hàn)**. Không còn track/plane F.Cu và không còn via. **Phải hàn thêm31đoạn dây cầu cách điện**, thuộc24 nhóm W1–W26 (bỏ W6 và W25), ở mặt hàn. Không phải bản một mặt không cần dây cầu. Các nét trên Dwgs.User là chỉ dẫn dây, không phải đồng mặt trên.

## Lưu ý trong KiCad và gia công

KiCad chuẩn hóa pad xuyên lỗ thành `*.Cu` và vẫn liệt kê hai lớp ngoài. Có thể thấy vành pad logic F.Cu; **không xuất/gia công F.Cu**. Mặt đồng chế tạo duy nhất là B.Cu. Không dựa riêng dòng “2 copper layers” của KiCad để đặt mạch hai mặt.

Các nhóm còn lại trong W1–W26 là footprint board-only, pad cùng số được đánh dấu nối nội bộ bằng jumper. DRC giả định dây đã lắp: **bỏ dây thực tế thì mạch sẽ thiếu nối dù báo cáo CAD bằng0**. Pad mô hình nhỏ trùng tại chân module không tạo thêm lỗ hay linh kiện mới. Điểm thay via cũ là bãi hàn dây trên B.Cu, không cần khoan thêm.

Tụ/IC trên carrier vẫn là chân cắm như trước; pad kiểuSMD trong nhómW chỉ để thể hiện bãi hàn dây. Dùng dây có vỏ cách điện, uốn gọn, không kéo căng và cố định chống rung; không dùng dây trần băng qua đường đồng. Hình thể hiện quan hệ nối, không ép tuyến dây thực tế phải thẳng.

Nhà gia công cần nhận yêu cầu rõ **single-sided, bottom copper only**,160×65mm; chốt độ dày đồng, vật liệu và kích thước lỗ thành phẩm. Thuộc tính drill PTH trong KiCad mô tả chân logic xuyên lỗ; nếu làm bo một mặt lỗ không mạ thì tất cả chân phải hàn vào đồng B.Cu. Không tự thêm đồng mặt trên chỉ để đáp ứng mạ lỗ.

## Hồ sơ lắp dây

- `SINGLE_LAYER_WIRES.csv`:31 cặp đầu dây, net, tọa độ mm, chân module liên quan.
- `SINGLE_LAYER_WIRES.json`:24 nhóm nối và các điểm hàn.
- `SINGLE_LAYER_PREVIEW.html`: chọn từng nhóm để xem điểm hàn; nhìn xuyên từ mặt linh kiện, chưa lật gương.
- `SINGLE_LAYER_OVERVIEW.png`: hình minh họa, không phải mask1:1.
- `Archive_2Layer_20261102/`: bản hai lớp và thư viện trước chuyển đổi.

Schematic chức năng giữ nguyên; dây cầu là cách thực hiện cùng net trên PCB, không thay pinout/rail hoặc firmware. PDF ướm module cũ chỉ đối chiếu cơ khí, không dùng ăn mòn/đặt gia công. Chưa xuất bộ Gerber cuối trong lượt này.

## Giới hạn cần xem trước khi chế tạo

Dây motor/pin và mối hàn phải chịu dòng tải thực; không dùng dây tín hiệu mảnh cho công suất. Một số bãi hàn thay via chỉ0,8–1,2mm, cần người lắp/xưởng xem khả năng hàn dây; có thể phải nới bãi hàn/đổi điểm đấu trước khi chốt. Chưa chứng nhận dòng định mức vì thiếu dòng tải và thông số đồng thành phẩm.

Giữ dây nguồn đi gọn gần nguồn/tải, tránh bó dây encoder/UART/I2C cùng dây motor. Hàn đủ31dây, kiểm thông mạch/chập/cực tính khi tắt nguồn, thử buck5V riêng trước MCU, sau đó kiểm3V3 và thử motor/cân bằng.

ERC/DRC/parity bản cuối:0vi phạm,0kết nối thiếu,0khác biệt schematic–PCB khi giả định jumper đã lắp. Đã đối chiếu kích thước, drill, net và vị trí tương đối của pad với bản trước khi dời U5. Không tắt kiểm tra điện để đạt kết quả.

Đây là phương án một mặt **có31dây cầu**, chưa tối ưu một mặt không dây. Giữ bố trí cũ làm số dây khá nhiều. Mất plane phía trên và thêm dây/mối hàn nên cần kiểm nguồn/tải/nhiễu lại; chưa chứng minh cân bằng tương đương breadboard hoặc bản hai lớp, chưa phê duyệt sản xuất hàng loạt.
