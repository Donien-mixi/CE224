# Lịch sử công cụ dựng xe

Các header đo motor/hiệu chuẩn nâng cao/LOADTEST cũ được chuyển khỏi thư mục firmware khi chốt RT10 MAX80. Chúng không được include hoặc cấp phát bộ nhớ trong bản vận hành. Giữ để đối chiếu kết quả thử và lịch sử; không chép lại vào sketch để nạp RT10.

Các script kiểm thử cũ dành cho12bài/LOADTEST có thể tham chiếu vị trí header cũ hoặc cố tình yêu cầu từ chốiMOVE8 preset2. Chúng là bài kiểm thử cho phiên bản cũ, không phải tiêu chí củaRT10. Với bản hiện hành chạy `tools/test_production_{controller,dashboard,session,gateway}.cjs`, cùng hồi quy recovery/tracking và mô phỏng chẩn đoán nêu trong báo cáoRT10.
