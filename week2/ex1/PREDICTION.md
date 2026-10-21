# Dự đoán — Week 2 / Ex1

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284

1. Phép tính kích thước queue tối thiểu (dùng `SAMPLE_PERIOD_MS`, thời gian xử lý
   ước lượng của `filter_task` và `uart_task` ở `UART_BAUD`):
**Trả lời:**
   - Với `SAMPLE_PERIOD_MS = 10 ms` tương đương **100 mẫu/s**. Filter tạo một kết quả cho mỗi mẫu nhận được, nên khi không mất mẫu, UART cũng nhận khoảng một mẫu với mỗi 10 ms.
   -  **Filter** xử lý tốn khoảng 0.1 ms/mẫu. Vì code chỉ cập nhật running sum của ba trục, ring buffer và tính trung bình; không cộng lại toàn bộ 16 mẫu. Đây là lượng xử lý nhỏ với CPU 72 MHz.
   **UART xử lý khoảng 6 ms/mẫu:** theo định dạng gửi `seq,ax,ay,az\r\n` dài tối đa 33 byte (10 ký tự sequence, ba số có dấu tối đa 6 ký tự, ba dấu phẩy và CR/LF). Với 8N1 (1 bit START + 8 bit dữ liệu + 0 bit parity + 1 bit STOP), thời gian truyền là `33 × 10 / 57600 ≈ 5.73 ms`.

   | Queue | Lập luận | Kích thước tối thiểu dự đoán |
   |---|---|---:|
   | `queue_raw` | Filter xử lý khoảng 0.1 ms, nhanh hơn nhịp mẫu đến 10 ms nên queue chỉ cần giữ một mẫu chờ nhận | **1 phần tử** |
   | `queue_filtered` | UART xử lý khoảng 6 ms, nhanh hơn nhịp kết quả đến 10 ms nên dự kiến sẽ không tích tụ mẫu qua các chu kỳ | **1 phần tử** |
   

   Kích thước tối thiểu dự đoán: `queue_raw` = 1 , `queue_filtered` = 1 .
   So với `QUEUE_LENGTH` được cấp: đủ hay không? Vì sao?
   - So với `QUEUE_LENGTH` được cấp dự kiến vẫn đủ. Vì độ đầy dự kiến của mỗi queue là `0 → 1 → 0`. 

2. Khi hạ `UART_BAUD` xuống 9600 (tạm thời), queue nào sẽ đầy trước? Vì sao?
**Trả lời:**
   - Em dự đoán `queue_filtered` sẽ đầy trước, với điều kiện sensor vẫn đọc mẫu gần 100 mẫu/s và filter xử lý kịp. UART mất nhiều thời gian hơn để truyền mỗi mẫu, song song với đó thì filter tiếp tục đưa kết quả vào queue_filtered.
   - Vì với baudrate 9600, 8N1 và chu kỳ 10 ms, số byte có thể truyền lý tưởng trong một chu kỳ là: 9600 × 0.010 / 10 = 9.6 byte/period
   - Định dạng truyền qua UART yêu cầu có ba dấu phẩy, hai byte CR/LF và ít nhất một chữ số cho mỗi trục. Khi sequence có ít nhất hai chữ số, mỗi dòng dài ít nhất: 2 + 3 × 1 + 3 + 2 = 10 byte nên T_UART_wire >= 10 × 10 / 9600 ≈ 10.417 ms. 
   - Do đó chỉ riêng thời gian truyền đã lớn hơn period 10 ms. Nếu sensor và filter tiếp tục tạo kết quả thoe chu kỳ 10 ms thì UART sẽ không theo kịp dẫn đến `queue_filtered` sẽ đầy. 

3. Với `xQueueSend(..., 0)` (timeout = 0) khi queue đầy: điều gì xảy ra với mẫu đó?
**Trả lời:**
   - Lời gọi trả về thất bại ngay, không chờ chỗ trống. Mẫu mới sẽ không được đưa vào queue, các mẫu cũ không bị tự động ghi đè. Kernel cũng không tự lưu mẫu mới để gửi lại sau. Với cấu hình hiện tại, mẫu đó bị bỏ và tăng bộ đếm gửi thất bại.
4. Với `xQueueSend(..., portMAX_DELAY)` khi queue đầy: `sensor_task` sẽ ở trạng thái
   nào? Có nguy cơ gì cho chu kỳ lấy mẫu?
**Trả lời:**
   -  Nếu sensor đổi sang gửi bằng `portMAX_DELAY` và `queue_raw` thực sự đầy, sensor ở trạng thái **Blocked**, chờ chỗ trống trong queue mà không có timeout. Task không dùng CPU để polling trong thời gian chờ.
   - Trong lúc chờ, sensor sẽ không đọc cảm biến cho kỳ tiếp theo. Nếu thời gian chờ kéo dài, chu kỳ lấy mẫu 10 ms có thể bị bỏ lỡ các lần đo tiếp theo.

5. Độ đầy `queue_raw` theo thời gian ở cấu hình bình thường sẽ trông như thế nào
   (tăng dần / dao động quanh một mức / luôn gần 0)? Vì sao?
**Trả lời:**
   - Như lập luận ở câu 1 của em, dự đoán `queue_raw` thường gần 0, có những khoảng ngắn lên 1 rồi trở về 0. Độ đầy sẽ không tăng dần kéo dài nếu filter xử lý kịp nhịp đầu vào từ queue_raw.
   - Sensor gửi mẫu → độ đầy tăng
   - Filter nhận mẫu → độ đầy giảm
   - Filter xử lý xong → quay lại chờ mẫu mới
   - Vì sensor chỉ tạo một mẫu mỗi 10 ms, còn filter dùng running sum với công việc cố định cho mỗi mẫu và được kỳ vọng theo kịp đầu vào. Với cấu hình bình thường không có nghẽn kéo dài, raw không tích tụ nhiều mẫu.

