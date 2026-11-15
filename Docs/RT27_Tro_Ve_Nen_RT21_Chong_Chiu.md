# RT27 — Trả khả năng chống chịu về nền RT21-ANTISTALL, bỏ cơ chế catch/return

Log dùng để sửa: `twip_balance_R12 (1) (3).txt` (12,4 s), `(36).txt` (16,1 s),
`(2) (3).txt` (5,2 s) — build **RT26-PUSH + NET27**, **không có lệnh lái** (`dir = turn = 0`),
CRC/gaps/reset = 0, `archive_error=1`.

Cặp bản nạp: **`BAL-20261115-R12-8DIR-RT27-RECOVER`** + **`…-RT27-RECOVER-NET28`**
(`.build/balance_release_r12_rt27_recover_20261115/`, `STM32_BAL_R12_RT27_RECOVER.bin`
sha256 `bd1c3889…`, `ESP32_RT27_RECOVER_NET28.merged.bin` `6c6c9a4b…`), trang
`http://192.168.4.1/ui/r12?v=RT27-RECOVER-NET28`.

## 1. RT26 sai ở đâu (đo trên 3 log này)

| | (1)(3) | (36) | (2)(3) |
|---|---|---|---|
| Kết thúc | **fault 4** (pitch −53°) | **fault 8** (cắt theo quãng) | **fault 4** (pitch −53°) |
| `v_ref` lúc ngã | **−0,87 m/s** | −0,70 | −0,82 |
| Vận tốc thật lúc đó | −0,87 | −0,68 | −0,85 |
| Pitch lúc cắt motor | −13,7 → −53° | **+13,8°** (còn dựng được) | −16,9 → −53° |
| `|x−x_ref|` | 0,05–0,14 m | **0,378 m** | 0,05 m |

Hai lỗi của tôi trong RT25/RT26:

1. **`v_ref := vận tốc thân` khi catch.** Vì `v_ref` bằng đúng vận tốc, số hạng
   `Kv·(v−v_ref)` trong luật R12 **bằng 0** — luật mất toàn bộ phanh theo vận tốc và
   tiếp tục *đẩy* xe theo hướng nó đang lăn. Log cho thấy xe bị **lái lùi tới 0,87 m/s**
   trong khi thân ngả về sau. Phát lại đúng cửa sổ trước lúc ngã: **tham chiếu RT21
   phanh mạnh hơn tới 5,3 N** so với catch (ví dụ ở v = −0,75: −6,95 N so với −2,37 N).
2. **REC1 chỉ còn +0,35 m** (tôi siết ở RT26). Log (36) bị **fault 8** ở
   `|x−x_ref| = 0,378 m` trong khi **thân mới 13,8° và xe đang phanh về** → cắt motor
   oan, rồi ngã.

## 2. RT27: quay về đúng nền RT21

**Gỡ bỏ hoàn toàn** `balance_catch.h`/`balance_push.h` (catch + drive-home assist) — không
còn `control_phase = 3`, không còn tham chiếu nào bám theo thân.

**Push recovery trở lại là REC1 của RT21-ANTISTALL**: góc 50°, tốc độ 1,2 m/s, quãng
`max(budget, |x−x_ref| + 0,8 m)` (đã trả lại 0,8 m), thoát khi yên, timeout 4 s, drive bị
inhibit.

Giữ **hai sửa đổi chỉ để tránh cắt motor oan** (không đổi luật cân bằng, không thêm chế độ):

- **RT23**: interlock tốc độ cần **200 ms liên tục** (RT21 cắt ở mẫu đầu tiên khi vượt
  0,80 m/s — một cú đẩy làm xe vượt 0,85 là đủ để cắt motor giữa lúc đang đỡ).
- **REC1 giữ `primed` sau khi thoát** (RT21 cần thêm 0,5 s yên mới nạp lại, nên cú đẩy thứ
  hai ngay sau một lần phục hồi không nâng được vỏ bọc).

Không đụng: gain/observer/allocator R12, luật chuyển động, luật bẻ hướng, governor RT22,
khung 122 B.

## 3. Kiểm chứng

- `tools/test_rt27_push.cjs` — phát lại **đúng 39 mẫu** trước cú ngã của log (1)(3):
  - `max |v_ref| = 0,000 m/s` khi không có lệnh lái (**RT26 để nó chạy tới 0,87**) → mốc
    vị trí và tham chiếu đứng yên như RT21.
  - Lực phanh mạnh nhất với tham chiếu RT21: **−9,57 N**; so với catch RT26 thì
    **chênh tới 5,3 N** ở cùng trạng thái.
  - Test fail nếu ai đó đặt lại `v_ref := v` (assert `|v_ref| ≤ 0,02`).
- `test_balance_recovery` trả lại kỳ vọng quãng **0,9 m** (0,3 + 0,8) của RT21.
- `test_rt18_launch` trả lại assert gốc "không nhảy mốc khi đứng yên".
- **27/27 host test pass**; 6 test `test_drive8_*`/`test_hardware_boot`/`test_load_speed`
  fail có sẵn từ trước.
- 12 ca mô phỏng stress chạy 20/20 s, không fault (đường mô phỏng không đổi vì catch đã gỡ).

## 4. Những gì **không** hứa được

- Cú đẩy đủ mạnh để thân vượt ~50° hoặc đẩy xe tới 0,9 m/s rồi thân ngả ngược thì **vẫn
  ngã**: ở tốc độ đó sức phản điện động chỉ còn ~1,5 N kéo tới. RT27 chỉ đảm bảo **không
  cắt motor oan** và **dùng hết lực** như RT21.
- Quãng trôi tối thiểu phụ thuộc ma sát sàn; RT21/RT27 giữ mốc vị trí nên xe tự quay lại
  trong khả năng của luật vị trí (đo trên log RT24: sai số đứng yên ~4–5 cm).
- **Chưa nạp lên xe.**

## 5. Việc cần làm trên xe

1. Nạp STM32 RT27-RECOVER + ESP32 NET28; bấm **SỬA VÙNG LOG FLASH** trước (vẫn
   `archive_error=1`).
2. Đứng yên, đẩy **nhẹ** 5 lần: kỳ vọng xe dao vài cm rồi **tự về mốc**, không có `fault`.
3. Đẩy **mạnh dần** tới khi ngã; ghi lại mức nào làm ngã + `pitch` lúc `fault 4`.
4. Đang chạy 50/72/80 thì va nhẹ: kiểm xe không mất ga và vẫn bám lệnh sau va.

## 6. Sự cố sau khi nạp (15/11): build ID lệch NGÀY — đã sửa

Triệu chứng người dùng báo: lên web **không chọn được ĐO LẠI GYRO** (và không bật được cân bằng).

Nguyên nhân: khi nâng bản, tôi đổi BALANCE_BUILD thành BAL-20261115-R12-8DIR-RT27-RECOVER
nhưng các literal trong gateway vẫn là **BAL-20261114-…** — lệch đúng phần **ngày**:

| Nơi | Chuỗi còn lại | Hậu quả |
|---|---|---|
| `dashboard_html.h` (2 chỗ: `matched()` + thông báo) | `BAL-20261114-R12-8DIR-RT27-RECOVER` | `matched()` = false → `canDrive()` false → **BẬT CÂN BẰNG và cần 8 hướng bị khoá**, banner "Chờ thông tin phiên bản STM32…" |
| `balance_web.h` (4 chỗ: `/operate` + header log) | như trên | `/operate` từ chối `arm` với 409 "STM32 chưa báo BAL-20261114-…" |
| `control_session.h` (2 chỗ) | như trên | khoá phiên điều khiển |
| `tools/test_net21_link.cjs` | như trên | test vẫn pass vì cùng chuỗi cũ (test không bắt được) |

**Đã sửa**: 9 literal → `BAL-20261115-R12-8DIR-RT27-RECOVER`; **thêm test gác**
`tools/test_build_id_sync.cjs`: đọc `BALANCE_BUILD` từ `balance_profile.h` rồi đòi **mọi**
literal `BAL-\d{8}-R12-8DIR-…` trong `ESP32_S3_Gateway/*.h|.ino` và `tools/*.cjs` (trừ
`test_drive8_*` cũ) phải trùng; có probe chứng minh lệch 1 ký tự là fail. Test này fail đúng
với lỗi vừa rồi.

**Chỉ cần nạp lại ESP32** (STM32 không đổi: `bd1c3889…`). Bản ESP32 mới:
`ESP32_RT27_RECOVER_NET28.app.bin` = `ec817195…`, `merged` = `f2b4dbe0…`.

**Vì sao ĐO LẠI GYRO bị mờ**: ngoài lỗi trên, nút này chỉ mở khi **motor đã dừng**
(`state ≠ 3/7`), telemetry còn mới, không có lệnh nào đang chờ, và phải mở mục
"Công cụ và chi tiết". Đứng yên + đã DỪNG thì nút mới bấm được.