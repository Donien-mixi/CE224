# RT36 — Sửa 4 lỗi đo được từ 4 log ngày 18/11/2026

> **RT37 (18/11) kế tiếp:** log RT36 `(2)(5)` cho thấy xuất phát/tăng tốc bị ngập ngừng do ngưỡng
> nghiêng governor 9°/11°; RT37 nâng thành 10°/13°. Xem [`RT37_Xuat_Phat_Muot.md`](RT37_Xuat_Phat_Muot.md).
> Các sửa của RT36 trong tài liệu này **vẫn giữ nguyên**.

Bốn log người vận hành gửi, cùng một bản **`BAL-20261117-R12-8DIR-RT34-CORNER` / gateway `…-NET35`**,
`reset=power_on`, `crc_errors=0`, `sequence_gaps=0`, `STM32_resets=0`, `drops=0`, `dt` 4 993–5 013 µs:

| Log | Nội dung | Mẫu | Thời lượng |
|---|---|---:|---:|
| `Tự đứng.txt` | đứng yên tự cân bằng | 7 526 | 150,5 s |
| `Kiểm tra rẽ trái phải của tiến…` | tiến + rẽ trái/phải | 5 689 | 113,8 s |
| `Kiểm tra rẽ khi chạy lùi…` | lùi + rẽ trái/phải | 7 259 | 145,2 s |
| `Chạy tiền lùi luân phiên…` | 90 cm/s tiến/lùi xen kẽ | 8 345 | 166,9 s |

RT36 chỉ sửa **lớp giám sát** (tham chiếu tốc độ, mốc vị trí, trần cua, ramp phanh). Không đổi gain,
observer, phân bổ lực R12, khung 122 B, lease, REC1, interlock hay trim 110 PWM.

## 1. Đo trước khi sửa

### 1.1 Đứng yên: dao động quanh một mốc cách xa

Cửa sổ 6 s, `state=3`, không lệnh: `x_ref = 0,000` suốt 60 s nhưng xe đứng ở **−0,05…−0,10 m**;
PWM giữ **+300** liên tục (có lúc −400), đảo dấu **2–14 lần/s**, `x` trượt qua lại **3–11 cm**
(stick-slip: lực `Kx·0,07 ≈ 0,64 N` chưa thắng ma sát nghỉ, thỉnh thoảng bật rồi overshoot).

### 1.2 Tốc 90: tiến chậm, lùi đủ

| Chiều | `v_ref` | `v` (p50) | duty p50 | lag | `x−x_ref` |
|---|---:|---:|---:|---:|---:|
| Tiến (8 lượt, 6–9 s) | 0,900 | **0,785–0,792** | 0,83 | **0,108** | **−0,100** (kẹp) |
| Lùi (7 lượt) | −0,900 | **−0,882…−0,915** | 0,90–0,95 | 0,015 | +0,02…+0,07 |

`x−x_ref` đúng −0,100 m ở mọi mẫu tiến = mốc bị `BalanceGovernor_Anchor` kẹp; duty 0,83 < 0,90 nên
bánh còn dư địa, không phải motor yếu. Governor `pressure` bật do **lag > 0,12**, và điều kiện xoá
(`lag < 0,08`) không bao giờ đạt → pressure chốt, mốc bị kẹp vĩnh viễn.

### 1.3 Cua: trái chậm hơn phải, phải-lùi ngập ngừng

| Điều kiện | `v_ref` | `v` | `w_ref` | `w` đo | %512 | pitch |
|---|---:|---:|---:|---:|---:|---:|
| Tiến + trái (36,6 s) | 0,799 | **0,611** | 1,15 | 1,15 | 0 % | **+6,1°** |
| Tiến + phải (18,2 s) | 0,692 | 0,679 | −1,15 | 1,14 | 67 % | +0,3° |
| Lùi + phải (40,6 s) | −0,802 | −0,798 | −1,15 | 1,12 | **87 %** | −1,6° |
| Lùi + trái (60,2 s) | −0,764 | −0,762 | 1,15 | 1,16 | 22 % | +4,3° |

Cua trái cũng bị kẹp `x−x_ref = −0,100` như mục 1.2 → không lên được 0,80. Cua phải-lùi có `|v|`
0,76–0,82 nên trần ngang tính theo **|v| đo** (`0,9/0,82 = 1,10`) cắt `w_ref` xuống 1,09–1,15 rồi
nhả → cờ 512 bật 87 % và `w_ref` không bao giờ đứng yên.

### 1.4 Nhả lệnh: trôi ~1 m

| Sự kiện | `v` lúc nhả | Thời gian về 0 | Quãng |
|---|---:|---:|---:|
| Lùi `spd90` 290,99 s | −0,92 | 1,72 s | ~1,0 m |
| Lùi `rev_turn` 976,35 s | −0,76 | 1,80 s | ~1,0 m |
| Tiến `spd90` 282,31 s | +0,84 | 1,56 s | ~0,9 m |

`v_ref` giảm 0,90 → 0 mất 1,56 s (≈0,57 m/s²) vì governor kẹp ramp phanh còn **0,65 m/s²** (motion cho
phép 0,90). Người vận hành cảm nhận là “không dừng ngay, còn trôi về sau”.

## 2. Sửa gì

| # | Tệp | Thay đổi | Vì sao an toàn |
|---|---|---|---|
| 1 | [`balance_governor.h:96-102`](../Xe_Can_Bang/Core/Inc/balance_governor.h) | `BalanceGovernor_Anchor` chỉ kẹp mốc khi **duty > 0,95**, không kẹp chỉ vì lag | Chỉ bỏ kẹp khi bánh còn dư; khi bão hoà thật (pin yếu) vẫn kẹp như cũ; detector kẹt tường + REC1 + trần quãng 0,30 m vẫn giữ |
| 2 | [`balance_turn.h:197`](../Xe_Can_Bang/Core/Inc/balance_turn.h) | trần ngang tính theo `guard = max(\|v_ref\|,\|v\|−0,10)` thay vì `\|v\|` đo | cùng quy tắc trần bánh RT23; tốc độ lệnh đã qua trần cua nên không thể nâng |
| 3 | [`balance_turn.h:63-64`](../Xe_Can_Bang/Core/Inc/balance_turn.h) | `BALANCE_TURN_VNL_BUDGET` 0,95 → **0,90** | cua 1,15 rad/s → 0,90−0,151 = **0,749 m/s** (yêu cầu 70–75 cm/s), bánh ngoài 0,90 < trần |
| 4 | [`balance_motion.h:232`](../Xe_Can_Bang/Core/Inc/balance_motion.h) | sau **3 s** đứng yên thật, mốc tự chảy về vị trí đo với **2 cm/s**, trong phạm vi **15 cm** | chỉ khi `\|v\|<0,02`, không phanh/không huỷ/không REC1; quá 15 cm coi là dịch chuyển thật nên vẫn giữ mốc cũ |
| 5 | [`balance_governor.h:36-37`](../Xe_Can_Bang/Core/Inc/balance_governor.h) | cap ramp phanh 0,65 → **0,85 m/s²** | vẫn dưới `BALANCE_MOTION_DECEL 0,90` và jerk 2,5; chỉ đổi tốc độ giảm tham chiếu, không đổi lực R12 |

## 3. Kiểm chứng

- **36/36 host test pass**, trong đó 4 test mới đều có **old-constant probe** phải fail:
  - `test_rt36_anchor` — duty 0,83 không kẹp; duty 0,96 kẹp ±0,10; probe `ANCHOR_DUTY=0` fail.
  - `test_rt36_lateral` — `w_ref` giữ 1,150 khi `|v|` dao động 0,76/0,82; 512 = 0/200 mẫu;
    probe `LATERAL_FROM_MEASURED=1` fail.
  - `test_rt36_hold` — đứng yên 6 s thì mốc về sát vị trí; đang chạy 0,30 m/s / REC1 / đã huỷ thì
    không chảy; probe `HOLD_RELAX=0` fail.
  - `test_rt36_brake` — 0,90 → 0 trong 1,30 s / 0,63 m; probe cap 0,65 fail.
- Test cũ đã cập nhật theo chính sách mới: `test_rt34_corner` (0,749 thay 0,799, probe 0,95),
  `test_rt22_turn` (trần ngang × guard ≤ 0,90).
- Mô phỏng (bộ 3×12 ca, chạy lại sau khi sửa): **12/12** ở MAX 80, **12/12** biên, **12/12**
  khởi hành 50, **11/12** ở MAX 90 — ca trượt vẫn là **pin 11,2 V** như bản RT35, không phát sinh ca mới.
- Build: STM32 83 152 B text / 0 lỗi 0 cảnh báo; ESP32 1 090 907 B. Hash và 13 tệp nguồn trong
  ``Firmware/RT36_SYM_20261118/``.

## 4. Chưa kiểm trên xe

Toàn bộ số ở mục 1 là log của bản **RT34**, không phải RT36. Cần một lượt đo lại cùng ma trận
(đứng yên 60 s; 90 tiến/lùi 8 s; cua trái/phải khi tiến và khi lùi; nhả lệnh từ 0,9) để xác nhận:
`v` tiến đạt 0,88–0,90 khi pin ≥ 12,0 V; cua hai chiều ~0,75; `w_ref` không còn nhảy; quãng trôi
khi nhả < 0,7 m; `x` khi đứng yên nằm trong ±3 cm.

## 5. Nạp

`Firmware/RT36_SYM_20261118/nap_rt36.bat` (nhập COM ESP32) — **nạp cả hai board**. Banner phải hiện
`RT36-SYM-NET36`, ba nút 50 / 80 / MAX 90. Giữ nguyên quy tắc: MAX 90 chỉ khi pin đo ≥ 12,0 V.
