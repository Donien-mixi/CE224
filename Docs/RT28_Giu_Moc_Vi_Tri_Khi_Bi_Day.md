# RT28 — Sửa: đẩy từ phía trước không quay về vị trí cũ

Log: `twip_balance_R12 (7)(2)`, `(8)(2)`, `(9)(2)` (đẩy ngay sau khi bật cân bằng — kết quả
được chấp nhận) và `(10)(2)`, `(11)(2)` (đã chạy các hướng, dừng xe, rồi mới đẩy — **đẩy từ
trước không hồi vị trí, đẩy từ sau thì hồi**). Build RT27-RECOVER + NET28.

Cặp bản nạp: **`BAL-20261115-R12-8DIR-RT28-ANCHOR`** + **`…-RT28-ANCHOR-NET29`**
(`.build/balance_release_r12_rt28_anchor_20261115/`, `STM32_BAL_R12_RT28_ANCHOR.bin`
sha256 `dc1f1410…`, `ESP32_RT28_ANCHOR_NET29.merged.bin` `e956f61d…`), trang
`http://192.168.4.1/ui/r12?v=RT28-ANCHOR-NET29`.

## 1. Nguyên nhân: REC1 lấy lại mốc vị trí ngay khi bắt đầu

Trong `BalanceTurn`, khi REC1 bắt đầu (lần đầu `recovering`):

```c
if(recovering&&!s->recovery_active&&s->used&&!s->cancel_flags){
  BalanceMotion_BeginBrake(s,position,now);   /* BeginBrake gán x_ref = position */
}
```

`BalanceMotion_BeginBrake` **gán `x_ref := vị trí hiện tại`**. Đo trên log:

| Log | t | `x_ref` trước → sau (1 tick) | xe ở đâu |
|---|---|---|---|
| (10)(2) | 54,34 → 54,36 | **+0,219 → +0,110** | x = +0,114 |
| (11)(2) | 37,98 → 38,02 | **−0,967 → −0,943** | x = −0,943 |

Tức là mốc cũ bị **xoá và thay bằng chỗ xe đang bị đẩy lệch** (10–11 cm). Sau đó xe hồi về
*chỗ mới đó*, không phải chỗ ban đầu.

**Vì sao chỉ đẩy từ trước mới lộ:** REC1 kích hoạt khi `|gyro| ≥ 150 °/s` hoặc nghiêng 8/12°.
Đẩy từ sau làm thân ngả nhanh → REC1 bật **sớm**, lúc xe mới lệch vài cm → mốc mới gần chỗ cũ
nên trông như "hồi đúng". Đẩy từ trước thân ngả chậm hơn → REC1 bật **muộn**, xe đã lệch
10–11 cm → mốc mới lệch hẳn → mất vị trí. Cùng một lỗi, chỉ khác thời điểm.

Thêm một chỗ thứ hai: sau khi phanh của REC1 kết thúc, mã cũ lại `x_ref=position` (cửa sổ
RT19) → nếu xe đã bị đẩy đi thì mốc lại bị chụp tại chỗ đã trôi.

## 2. Sửa (chỉ trong tầng tham chiếu chuyển động)

1. `balance_motion.h`: thêm cờ **`hold_anchor`** và hàm
   `BalanceMotion_BeginBrakeHold(s,now)` = `BeginBrake(s, s->x_ref, now)` — tức là **bắt đầu
   phanh mà KHÔNG chụp lại mốc**.
2. `balance_turn.h`: chỗ REC1 bắt đầu dùng `BalanceMotion_BeginBrakeHold` thay cho
   `BeginBrake(s,position,…)`.
3. Khi `hold_anchor` bật: **không** theo vị trí trong lúc phanh (`x_ref=position`) và **không**
   chụp lại mốc ở cửa sổ RT19; cờ được xoá khi có lệnh lái mới hoặc khi phanh kết thúc.
4. **Không đổi**: lệnh lái mới vẫn chụp mốc tại chỗ bắt đầu; nhả ga có lệnh vẫn chụp mốc tại
   chỗ dừng (hành vi RT19); REC1/interlock/governor/luật R12 giữ nguyên.

## 3. Kiểm chứng

`tools/test_rt28_anchor.cjs` (chạy tầng motion thật):

| Ca | Kết quả |
|---|---|
| Đã từng lái, xe đứng ở mốc 0, bị đẩy −0,10 m rồi −0,28 m, REC1 bật | `x_ref` = **0,0000** suốt (trước khi sửa: nhảy sang chỗ bị đẩy) |
| Phanh REC1 kết thúc, xe yên | `x_ref` = **0,0000** (vẫn là mốc cũ) |
| Cú gọi cũ `BeginBrake(&c, −0,28)` trên bản sao | `x_ref` 0,500 → **−0,280** ⇒ test bắt đúng lỗi |
| Lệnh lái mới tại x = +0,50 | `x_ref` = **0,500** (vẫn chụp mốc mới) |
| Nhả ga có lệnh, dừng ở +0,30 | `x_ref` = **0,300** (hành vi RT19 giữ nguyên) |

- **29/29 host test pass** (6 test `test_drive8_*`/`test_hardware_boot`/`test_load_speed` fail
  có sẵn từ trước). `test_build_id_sync` pass (9 literal khớp `BALANCE_BUILD`).
- Log 7/8/9 (đẩy ngay sau khi bật cân bằng) **không đổi hành vi**: lúc đó `used=false` nên mã
  cũ cũng không chụp lại mốc (`x_ref` = 0,000 suốt) — đúng như kết quả bạn đã chấp nhận.

## 4. Chưa kiểm / cần thử trên xe

- **Chưa nạp RT28.** Kỳ vọng: sau khi chạy các hướng rồi dừng, đẩy **từ trước** xe về đúng chỗ
  dừng (sai số vài cm), đẩy **từ sau** vẫn tốt như trước.
- Giới hạn lực ở ~0,9–1,0 m/s vẫn như đã báo (trần điện áp/phản điện động); RT28 chỉ sửa
  **mốc vị trí**, không thêm lực.
- Nếu sau khi nạp mà đẩy từ trước vẫn lệch >10 cm, gửi log: cần xem `x_ref` có nhảy ở chỗ nào
  khác (cửa sổ RT19 hoặc `BalanceMotion_Cancel`).
