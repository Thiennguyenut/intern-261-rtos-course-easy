# Dự đoán — Week 3 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284

1. Thời gian truyền 1 byte ở `RX_BAUD` của bạn là bao nhiêu us? (8N1, tính cả
   start/stop bit). Trong thời gian đó `parser_task` có kịp xử lý 1 byte từ
   stream buffer không, nếu nó là task ưu tiên cao nhất hệ thống?
   **Trả lời:**
   - Với UART 8N1, mỗi byte chiếm 10 bit trên đường truyền, gồm: 1 start bit, 8 data bit và 1 stop bit. Ở 115200 baud, thời gian truyền là 10 / 115200 ≈ 86,81 µs/byte tương đương 6.250 chu kỳ ở 72MHz.
   - Em dự đoán parser theo kịp. Vì với phần lớn byte, ISR chỉ lưu byte và bật lại nhận, còn parser chỉ đọc, kiểm tra và lưu vào mảng. Parser ưu tiên cao nên được chạy sớm khi đã Ready và có thể đọc liên tiếp các byte đang chờ. Nó chỉ phân tích lệnh khi gặp \r. Buffer hấp thụ độ trễ ngắn ở bước này, miễn là tổng thời gian CPU trung bình để nhận và xử lý mỗi byte dưới 86,8 µs.

2. `STREAM_BUFFER_BYTES` của bạn chứa tối đa bao nhiêu lệnh dài `MAX_CMD_LEN` byte trước khi đầy?
   **Trả lời:**
   - Vì `MAX_CMD_LEN` = 24 byte (gồm 23 byte nội dung và 1 byte /r) nên Buffer 64 byte chứa được 2 lệnh đầy đủ, còn 64 − 2 × 24 = 16 byte cho một phần lệnh tiếp theo. Nếu gửi liên tục từ lúc buffer rỗng và parser chưa đọc, buffer đầy sau khoảng 64 × 86,81 µs ≈ 5,56 ms tính từ lúc bắt đầu truyền.

3. Nếu `xStreamBufferSendFromISR` trả về 0 (stream buffer đầy) trong
   `uart_rx_isr_handler`, byte đó sẽ bị mất. Theo bạn, trường hợp nào thực tế khiến điều này xảy ra (gửi quá nhanh / `parser_task` ưu tiên quá thấp /
   cái khác)?
   **Trả lời:**
   - Trường hợp là parser ưu tiên quá thấp hoặc parser xử lý lệnh hay ghi log quá lâu trong lúc byte vẫn đến. Ví dụ như parser bận ghi log đồng bộ 6 ms trong lúc script gửi liên tục. IRQ vẫn ghi byte vào buffer. Từ buffer rỗng, 64 byte đầu làm đầy buffer sau khoảng 5,56 ms. Khi byte thứ 65 đến, hàm trả về 0 và byte đó bị mất. 

4. Dự đoán: gửi lệnh bằng gõ tay bình thường có mất byte không? Gửi bằng
   script gửi liên tục không nghỉ thì sao?
   **Trả lời:**
   - Em dự đoán gõ tay bình thường không mất byte vì giữa các lần gõ có khoảng nghỉ. Riêng với trigger 16 byte và portMAX_DELAY, lệnh ngắn như P10\r có thể đã nằm đủ trong buffer mà parser vẫn Blocked. Để lệnh ngắn phản hồi ngay, cần đánh thức parser khi ghi thành công \r. 
   - Với script gửi liên tục, em dự đoán parser theo kịp nếu tốc độ rút byte trung bình đủ cao và buffer còn chỗ trong các đợt parser bị trễ. 
