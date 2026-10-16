1. Tích hợp FreeRTOS
Kernel được đặt tại third_party/FreeRTOS-Kernel và quản lý bằng Git submodule.

Trong CMake:

Dùng biến EXERCISE để chọn week1/ex1.

Tạo target freertos_config, trỏ đến thư mục app/ của exercise.

Chọn FREERTOS_PORT = GCC_ARM_CM3.

Chọn FREERTOS_HEAP = 4.

Thêm kernel bằng add_subdirectory.

Thêm app_main.c vào source và liên kết freertos_kernel.
2. Build và kiểm tra API
Trong PowerShell:
```
cd D:\intern-261-rtos-course-easy\template
cmake --preset Debug -DEXERCISE=week1/ex1
cmake --build --preset Debug
```
File chương trình được tạo tại: template/build/Debug/template.elf
Bản build dùng để đo báo RAM 5952 byte và Flash 12056 byte.
Trong Git Bash, tại gốc repo:
```
bash tools/check_api.sh
echo $?
```bash
3. Hai lỗi thực tế đã gặp

Lỗi 1: Trùng định nghĩa handler

Linker báo multiple definition cho SVC, PendSV và SysTick.

Nguyên nhân: sau khi map tên, cả port FreeRTOS và stm32f1xx_it.c cùng định nghĩa ba handler.

Cách sửa:

Vào CubeMX → System Core → NVIC → Code generation.

Bỏ chọn Generate IRQ handler cho SVC, PendSV và SysTick.

Giữ handler TIM4.

Sinh lại code và build lại.

Kết quả: hết lỗi trùng định nghĩa. Log trước khi sửa nằm trong linker_handler_conflict.txt.

Lỗi 2: Công cụ nạp không tìm thấy ELF

CLI kết nối được MCU nhưng báo:

Error: File does not exist: template.elf

Kiểm tra cho thấy ELF không tồn tại tại đường dẫn template/build/Debug/template.elf.

Cách sửa:

Cấu hình và build lại bằng preset Debug.

Dùng Test-Path kiểm tra file đã tồn tại.

Nạp bằng đường dẫn tuyệt đối đến ELF.