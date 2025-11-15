#include "kernel/types.h" // định nghĩa các kiểu dữ liệu cơ bản của xv6
#include "kernel/fs.h"    // cấu trúc hệ thống file (dirent, DIRSIZ, ...)
#include "kernel/stat.h"  // cấu trúc stat, kiểu file (T_DIR, T_FILE, ...)
#include "user/user.h"    // khai báo các syscall phía user (open, read, printf, ...)

// hàm lấy phần tên file từ một đường dẫn đầy đủ.
// ví dụ: "/a/b/c" -> "c"
static char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];  // buffer để lưu tên file (tối đa DIRSIZ ký tự + 1 ký tự kết thúc chuỗi)
  char *p;

  // duyệt từ cuối chuỗi path về đầu, tìm dấu '/' cuối cùng.
  // sau vòng lặp, p sẽ dừng tại vị trí ngay trước dấu '/' cuối cùng.
  for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;  // ++p để p trỏ đúng vào ký tự đầu tiên sau '/'

  // nếu độ dài tên file >= DIRSIZ thì trả thẳng con trỏ p,
  // không copy vào buf nữa (tránh tràn).
  if(strlen(p) >= DIRSIZ) return p;
  
  // ngược lại, copy tên file vào buf và thêm ký tự kết thúc chuỗi.
  memmove(buf, p, strlen(p)); 
  buf[strlen(p)] = 0; 

  return buf;   // Trả về chuỗi tên file
}

// hàm đệ quy: tìm trong cây thư mục bắt đầu từ 'path' tất cả file có tên = 'target'
static void find(char *path, char *target)
{
  char buf[512], *p; // buf: buffer ghép đường dẫn con, p: con trỏ chạy trên buf
  int fd;            // fd: file descriptor của thư mục hiện tại
  struct dirent de;  // de: entry thư mục (thông tin file / thư mục con)
  struct stat st;    // st: thông tin chi tiết của file / thư mục (loại, kích thước, ...)

  // mở đường dẫn hiện tại
  if((fd = open(path, 0)) < 0)
  { 
    fprintf(2, "find: cannot open %s\n", path); 
    return; // không mở được thì dừng tại đây
  }

  // lấy thông tin stat của path
  if(fstat(fd, &st) < 0)
  { 
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd); 
    return; 
  }

  // nếu path hiện tại KHÔNG phải là thư mục
  if(st.type != T_DIR)
  {
    // so sánh tên file với target.
    // nếu trùng thì in ra đường dẫn đầy đủ.
    if(strcmp(fmtname(path), target) == 0) printf("%s\n", path);
    close(fd); 
    return;
  }

  // nếu là thư mục, kiểm tra độ dài dự kiến của đường dẫn con xem có vượt quá buf không
  // path + '/' + tên file (DIRSIZ) + '\0'
  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
  {
    fprintf(2, "find: path too long\n"); 
    close(fd); 
    return;
  }

  // copy path hiện tại vào buf
  strcpy(buf, path); 
  p = buf + strlen(buf); // p trỏ tới ký tự '\0' cuối chuỗi
  *p++ = '/'; // thêm '/' và tăng p lên vị trí sau '/'

  // đọc từng entry trong thư mục
  while(read(fd, &de, sizeof(de)) == sizeof(de))
  {
    if(de.inum == 0) continue; // inum = 0 nghĩa là entry trống, bỏ qua

    // bỏ qua 2 thư mục đặc biệt "." và ".." để tránh đệ quy vô hạn
    if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) 
      continue;
    
    // copy tên entry vào sau dấu '/' trong buf.
    memmove(p, de.name, DIRSIZ); 
    p[DIRSIZ] = 0; // kết thúc chuỗi tại vị trí sau tên file/thư mục
    
    // lấy stat của đường dẫn con (buf = path + "/" + de.name)
    if(stat(buf, &st) < 0) 
      continue;

    if(st.type == T_FILE)
    { 
      // nếu là file thường, so sánh tên (p đang trỏ tới tên file trong buf)
      if(strcmp(p, target) == 0) 
        printf("%s\n", buf); // in ra đường dẫn đầy đủ nếu trùng tên
    }
    else if(st.type == T_DIR)
    { 
      // nếu là thư mục con: đệ quy tiếp tục tìm trong thư mục đó
      find(buf, target); 
    }
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  // argv[1]: đường dẫn bắt đầu tìm
  // argv[2]: tên file cần tìm
  if(argc != 3)
  { 
    fprintf(2, "usage: find <start-path> <filename>\n"); 
    exit(1); 
  }

  // gọi hàm find với đường dẫn bắt đầu và tên file mục tiêu
  find(argv[1], argv[2]); 
  exit(0);
}