# Dự đoán — Week 3 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284

1. Với nút nhấn cơ khí thông thường, nảy phím thường kéo dài bao lâu
   (tra cứu/ước lượng)? So với `DEBOUNCE_MS` của bạn: đủ lớn để lọc hết không?
   **Trả lời:**
   - Theo phép đo của Jack Ganssle, 16 trong 18 công tắc có thời gian nảy trung bình khoảng 1,6 ms và tối đa 6,2 ms trong nhóm đó. Hai mẫu còn lại từng nảy lâu hơn. 
   - Em dự đoán 15 ms đủ để lọc các cụm nảy ngắn vài ms trên nút.

2. Nếu `DEBOUNCE_MS` quá lớn, điều gì xảy ra khi người dùng nhấn nhả rất
   nhanh liên tiếp (double-click)?
   **Trả lời:**
   - Nếu nhấn lần thứ hai trước khi khoảng nhả ở giữa đủ dài để được xác nhận, cạnh nhấn mới sẽ reset one-shot. Trạng thái đã xác nhận vẫn là nhấn từ lần đầu. Khi timer hết hạn, nút vẫn được đọc là nhấn nên không có chuyển trạng thái nhả → nhấn mới. 
   - Vì vậy, hai lần nhấn có thể chỉ tạo một PRESS. 

3. Khi callback của `debounce_timer` bị block 50 ms (thử nghiệm bước 4), bạn
   dự đoán `longpress_timer` sẽ bị trễ bao nhiêu? Giải thích bằng cơ chế
   daemon task + queue lệnh, không chỉ đoán số.
   **Trả lời:**
   - Em dự đoán HELD có thể trễ tối đa khoảng 50 ms do block nếu đến hạn ngay khi debounce callback bắt đầu delay. 
   - Vì hai callback chạy lần lượt trên cùng một daemon task. Khi daemon bị block, các lệnh trong timer command queue và callback đến hạn đều phải chờ. Ngoài ra nếu nhấn giữ bình thường, daemon dự kiến block khoảng 15–65 ms, còn HELD đến hạn ở 600 ms, nên HELD không bị trễ thêm do delay.

4. `LONGPRESS_MS` của bạn là bao nhiêu? Giữ phím đúng ngưỡng này, bạn dự đoán
   callback `longpress_callback` chạy lần đầu ở thời điểm nào tính từ lúc
   nhấn xuống?
   **Trả lời:**
   - LONGPRESS_MS của em là 600 ms. Nếu nút vẫn được giữ, em dự đoán HELD đầu tiên xuất hiện khoảng 600 ms sau cạnh nhấn cuối cùng. ISR khởi động long-press timer ngay tại cạnh đó, nên 600 ms được đếm song song với cửa sổ debounce 15 ms. Nếu nhả đúng sát ngưỡng, việc có HELD hay không còn phụ thuộc vào thứ tự daemon xử lý mốc hết hạn và các lệnh.