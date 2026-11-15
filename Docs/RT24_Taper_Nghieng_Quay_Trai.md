# RT24 — Quay trái vẫn chậm hơn quay phải: phần mềm hay phần cứng?

Log kiểm tra: `twip_balance_R12 (1).txt` (3 277 mẫu, 65,5 s, `BAL-…-RT23-SYM` +
`NET24`, CRC/gaps/reset = 0, `archive_error=1`). Cặp bản sửa: **RT24-SYM / NET25**.

## 1. Số đo trên log mới (bản RT23 đã sửa taper tâm quay)

| Hướng quay tại chỗ, preset 80 | w_ref (yêu cầu 2,40) | yaw đo được | pitch | trôi tâm |
|---|---|---|---|---|
| phải (`turn=−1`) | **2,40** phẳng, p10 2,40 | 2,41 | **−2,19°** | **3,9 cm** |
| trái (`turn=+1`) | **2,18** median, p10 **1,65**, min 1,56 | 2,09 | **+6,54°** (p90 7,87, max 8,9) | **16,9 cm** |

58 % số mẫu quay trái nằm dưới 2,38 rad/s, quay phải chỉ 18 %. Vậy bản RT23 đã sửa
được taper tâm quay nhưng **vẫn còn một chỗ ghìm khác**.

## 2. Tìm ra chỗ ghìm: taper nghiêng dùng pitch tuyệt đối

`balance_turn.h` vẫn còn `balance=(12−|pitch|)/6` — tức là bắt đầu giảm tốc độ quay
khi thân nghiêng quá **6°**. Nhưng một cú xoay tại chỗ 2,40 rad/s của chính xe này
**luôn** nghiêng: +6,5° khi quay trái và −2,2° khi quay phải. Nên:

| pitch | taper | w_ref |
|---|---|---|
| −2,2° (phải) | 1,00 | 2,40 |
| +6,5° (trái, median) | 0,92 | 2,21 |
| +7,9° (trái, p90) | 0,69 | 1,66 |
| +8,9° (trái, max) | 0,52 | 1,24 |

Số tính ra khớp đúng số đo (2,18 và 1,65). **Đây là lỗi phần mềm:** luật giảm tốc
quay lẽ ra phải xét độ nghiêng *vượt quá tư thế trung tính*, không phải độ nghiêng
tuyệt đối. RT22 đã sửa đúng lỗi này cho governor (ngưỡng nghiêng 5° → 9° tính từ
tư thế trung tính) nhưng bộ phận bẻ hướng thì chưa.

## 3. Phần cứng: có, và nó là nguyên nhân của độ nghiêng

Vì sao quay trái thân lại ngả +6,5° còn quay phải −2,2°? Đó là **bất đối xứng cơ khí**,
không phải luật điều khiển:

- Ở cùng tốc độ quay, bánh **trái luôn chạy nhanh hơn** bánh phải: quay phải
  `v_l=+0,331 / v_r=−0,295`; quay trái `v_l=−0,286 / v_r=+0,261` (|trái| > |phải|
  cả hai chiều, chênh 0,02–0,04 m/s).
- Duty cũng lệch theo: `pwm` trái ±954/±879 so với phải ∓918/∓858 — bánh trái cần
  nhiều điện áp hơn cho cùng tốc độ ⇒ **ma sát/trở kháng cơ khí của cụm trái cao hơn**.
- Hệ quả: trong cú xoay, hai bánh tạo ra một lực dọc **không triệt tiêu hết**; thân xe
  phải ngả về phía trước để bù (và vì thế xe bò tới **16,9 cm** khi quay trái, chỉ
  3,9 cm khi quay phải — vị trí R12 kéo lại nhưng dừng ở mức lệch đó).
- Ngay cả khi `w_ref = 2,40` phẳng, yaw đo được vẫn còn **2,285 (trái) so với 2,378
  (phải) = −4 %** — phần này là giới hạn lực kéo của cơ cấu, không phải luật.

## 4. Đã sửa (RT24) và cách kiểm chứng

**Sửa:** taper nghiêng dùng `|pitch − tư thế trung tính|` (giá trị học được, BINFO
`+1,074°`, `qualified=1`): **toàn quyền tới 8° vượt trung tính**, giảm tuyến tính
8→12°, bằng 0 khi vượt 12°. Không đổi gain/observer/allocator R12, governor, REC1,
trần bánh theo preset, trần ngang 0,9, knee tâm quay 45 cm, blockage, khung 122 B.

- `test_rt24_sym.cjs` — **phát lại chính trace pitch đo được trên xe** (117 mẫu trái,
  92 mẫu phải, giữ 4 tick/mẫu = đúng 20 ms/sample như log) qua `BalanceMotion8_Update`:
  - RT24: trái **2,40 / min 2,40**, phải **2,40 / min 2,40**.
  - Luật RT23 (pitch tuyệt đối, `(12−|p|)/6`): trái **1,95 median, 1,37 min**, phải
    2,40 → probe **phải fail** và đã fail. Đúng như log người dùng.
- `test_rt22_turn.cjs` cập nhật ca "lean 10°" theo luật mới (10° tuyệt đối với tư thế
  trung tính 0° ⇒ 0,50 ⇒ 1,20 rad/s; nếu tư thế trung tính +1,07° thì còn 8,9° vượt
  ⇒ giữ 2,40).
- 26/26 host test pass (6 test `test_drive8_*`/`test_hardware_boot`/`test_load_speed`
  fail có sẵn từ trước, đọc mock cũ).
- 36 ca mô phỏng 20/20 s, không fault; ca chéo 6/12 tăng nhẹ (759° so với 747°),
  ca 9/10 tăng 8° — đúng hướng nới taper, không có ca nào xấu đi.

## 5. Kỳ vọng trên xe và việc kiểm phần cứng

Sau RT24: `w_ref` quay trái phải bằng quay phải (2,40), tốc độ quay đo được còn lệch
**~4 %** và **trôi tâm 17 cm khi quay trái vs 4 cm khi quay phải**.

Muốn xử lý phần cứng, làm theo thứ tự rẻ nhất:

1. **Đổi chỗ hai motor** (trái ↔ phải, giữ nguyên dây encoder/động cơ theo nhãn) rồi
   quay lại 8 s mỗi bên. Nếu độ lệch pitch/trôi **đi theo motor** ⇒ do motor/hộp giảm
   tốc; nếu vẫn ở phía bên trái khung ⇒ do khung/bánh/ổ trục.
2. Kiểm **đường kính và độ mòn bánh** hai bên (lệch >1 % là đủ gây lệch yaw), áp lực
   ổ trục bánh trái, và xem có cọ dây/ga-ra không.
3. Nếu đổi motor không hết: đo dòng từng motor khi quay tại chỗ (cần kẹp dòng) — dữ
   liệu này hiện **chưa có** trong log (firmware không đo dòng).

## 6. Chưa kiểm / rủi ro

- **Chưa nạp RT24 lên xe.** Toàn bộ kết luận "phần mềm" dựa trên log + phát lại.
- Nới taper nghiêng làm cú quay ít bị ghìm hơn khi thân đang ngả 6–8°: nếu sàn trơn
  và xe bắt đầu ngã trong lúc quay, thời điểm can thiệp muộn hơn ~2° so với RT23
  (vẫn còn ngưỡng REC1 12° và tilt 25° phía sau).
- `archive_error=1` vẫn còn trong log mới ⇒ bản RT24 vẫn **mất log nếu ESP32 reset**;
  phải bấm SỬA VÙNG LOG FLASH trước buổi đo.
