typedef unsigned int uint; // định nghĩa uint cho file user/user.h 
#include "user/user.h"     // syscall phía user: read, write, fork, exec, wait, exit, ...
#include "kernel/param.h"  // MAXARG: số lượng tham số tối đa cho exec

int main(int argc, char *argv[])
{
    int argc2 = argc - 1; // bỏ qua \.xargs, là số đối số thực tế của exec()
    char *argv2[MAXARG];  // mảng tham số sẽ truyền cho exec

    // sao chép vào argv2
    // ví dụ: "xargs echo bye" -> argv2[0] = "echo", argv2[1] = "bye"
    for (int i = 1; i < argc; i++)
    {
        argv2[i - 1] = argv[i];
    }

    char s[64]; // buffer để đọc từng "từ" từ stdin
    int c = 0;  // chỉ số hiện tại trong buffer s
    int not_ended = 0; // biến cờ: 1 = chưa kết thúc dòng (chưa gặp '\n'), 0 = vừa gặp '\n'

    // đọc từng ký tự từ stdin (fd = 0) và lưu vào s[c]
    while (read(0, &s[c++], sizeof(char)))
    {
        // kiểm tra ký tự vừa đọc:
        // - nếu khác '\n' => vẫn đang trong cùng 1 dòng, set not_ended = 1
        // - nếu là '\n'   => kết thúc một dòng, set not_ended = 0
        if (s[c - 1] != '\n') 
            not_ended = 1;
        else 
            not_ended = 0;
        
        // nếu ký tự vừa đọc KHÔNG phải khoảng trắng và vẫn chưa hết dòng, thì tiếp tục đọc, chưa tách "từ"
        // ở đây ta coi dấu ' ' là dấu phân cách giữa các từ trong cùng một dòng
        if (s[c - 1] != ' ' && not_ended) continue;
        
        // đến đây là:
        // - hoặc vừa đọc được ' ' trong một dòng
        // - hoặc vừa đọc được '\n' (kết thúc dòng)
        // ta sẽ kết thúc chuỗi s tại vị trí này để được 1 "từ" hoàn chỉnh.
        s[c - 1] = '\0';

        // cấp phát vùng nhớ động để lưu token vừa đọc
        argv2[argc2] = (char *)malloc(c);  // c là số ký tự đã đọc (bao gồm '\0')
        strcpy(argv2[argc2++], s); // copy "từ" vào argv2[argc2], rồi tăng argc2
        c = 0; // reset chỉ số buffer để đọc "từ" tiếp theo

        // nếu vẫn chưa hết dòng (tức vừa gặp ' ' chứ chưa gặp '\n') 
        // thì chỉ mới thêm 1 tham số cho lệnh, tiếp tục đọc để lấy thêm
        if (not_ended) continue;

        // đến đây nghĩa là vừa gặp '\n' -> đã có đủ tất cả "từ" của 1 dòng
        // đặt phần tử cuối = 0 để tạo mảng argv hoàn chỉnh cho exec()
        argv2[argc2] = 0;

        // tạo tiến trình con để chạy lệnh với các tham số đã gom được
        if (fork() == 0)
        {
            exec(argv2[0], argv2);
            // Nếu exec thất bại, thoát để tránh chạy tiếp code phía dưới
            exit(0);
        }
        else
            // tiến trình cha đợi con kết thúc trước khi xử lý dòng tiếp theo
            wait(0);

        // reset argc2 về số tham số ban đầu của lệnh sau khi xử lý xong 1 dòng
        // để chuẩn bị cho dòng tiếp theo.
        argc2 = argc - 1;
    }

    // khi đọc EOF từ stdin thì thoát chương trình
    exit(0);
}