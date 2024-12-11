#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

typedef char i8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef unsigned long u64;

#define PRINT_ERROR(cstring) write(STDERR_FILENO, cstring, sizeof(cstring) - 1)
#define HEADER_LEN 296

// B G R 0
// B G R = ascii chars
// Message len: R + B
// Max len 510
// pixels are stored starting from the lower left, and ends at the top right of the image

#pragma pack(1)

typedef struct bgr {
    i8 b;
    i8 g;
    i8 r;
    i8 z;
} bgr_t;


struct bmp_header
{
	// Note: header
	i8  signature[2]; // should equal to "BM"
	u32 file_size;
	u32 unused_0;
	u32 data_offset;

	// Note: info header
	u32 info_header_size;
	u32 width; // in px
	u32 height; // in px
	u16 number_of_planes; // should be 1
	u16 bit_per_pixel; // 1, 4, 8, 16, 24 or 32
	u32 compression_type; // should be 0
	u32 compressed_image_size; // should be 0
    
	// Note: there are more stuff there but it is not important here
};

// void print_px_d(i8* pos, u32 rmask, u32 gmask, u32 bmask) {
//     unsigned char c;
//     for (u16 j = 0; j < 3; j++){
//         c = *(pos + j);
//         write(1, &c, 1);
//     }
// }


struct file_content
{
	i8*   data;
	u32   size;
};

struct file_content   read_entire_file(char* filename)
{
	char* file_data = 0;
	unsigned long	file_size = 0;
	int input_file_fd = open(filename, O_RDONLY);
	if (input_file_fd >= 0)
	{
		struct stat input_file_stat = {0};
		stat(filename, &input_file_stat);
		file_size = input_file_stat.st_size;
		file_data = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_file_fd, 0);
		close(input_file_fd);
	}
	return (struct file_content){file_data, file_size};
}

int read_row(i8 *src, i8 *dst, u8 real_bytes_to_read) {
    // printf("row len %d\n", real_bytes_to_read);
    u8 i = 0;
    u8 j = 0;
    // write(1, "[", 1); 
    while (i < real_bytes_to_read && j <= 24) {
        if ((j&3) == 3) {
            j++;
            continue;
        }
        dst[i] = src[j];
        // write(1, dst + i, 1);

        i++;
        j++;
    }
    // write(1, "]\n", 2);
    return j;
}

i8 is_root_px(u8 *px) {
    if (px[0] == (u8)127) {
    
    }
    if (px[0] == (u8)127 && px[1] == (u8)188 && px[2] == (u8)217) {
        // printf("[%u %u %u %u]", px[0], px[1], px[2], px[3]);
        return 1;
    }
    return 0;
}

i32 find_root_row(u8 *data, u32 len) {
    u32 cur_pos = 0;
    while (cur_pos <= len - 8){
        if (is_root_px(data)) {
            // printf("Found square %d\n", cur_pos);
        }
        if (is_root_px(data) && is_root_px(data + 4)  && is_root_px(data + 8)  && is_root_px(data + 12) &&
        // if (is_root_px(data) && is_root_px(data + 1)  && is_root_px(data + 2)  && is_root_px(data + 3)){
            is_root_px(data + 16) && is_root_px(data + 20) && is_root_px(data + 24)) {
                return cur_pos;
            }
        cur_pos++;
        data++;
    }
    return -1;
}

i8 check_root_leg(u8 *data, u32 w) {
    if (is_root_px(data - w)  && is_root_px(data - 2*w)  && is_root_px(data - 3*w) &&
            is_root_px(data - 4*w) && is_root_px(data - 5*w)  && is_root_px(data - 6*w) &&
            is_root_px(data - 7*w))
            return 1;
    return 0;
}

u8 *find_root(u8 *data, i32 data_len, u32 w) { // read in chunks of 4
    i32 cur_pos = 0;
    while (cur_pos < data_len) {
        cur_pos = find_root_row(data + cur_pos, data_len - cur_pos);
        if (cur_pos == -1) {
            // printf("root row(\n");
            break;
        }
        if (check_root_leg(data + cur_pos, w)){
            return (data + cur_pos);
        } else {
            // printf("leg\n");
        }
    }
    return NULL;
}


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		PRINT_ERROR("Usage: decode <input_filename>\n");
		return 1;
	}
	struct file_content file_content = read_entire_file(argv[1]);
	if (file_content.data == NULL)
	{
		PRINT_ERROR("Failed to read file\n");
		return 1;
	}
	struct bmp_header* header = (struct bmp_header*) file_content.data;
	// printf("signature: %.2s\nfile_size: %u\ndata_offset: %u\ninfo_header_size: %u\nwidth: %u\nheight: %u\nplanes: %i\nbit_per_px: %i\ncompression_type: %u\ncompression_size: %u\n", header->signature, header->file_size, header->data_offset, header->info_header_size, header->width, header->height, header->number_of_planes, header->bit_per_pixel, header->compression_type, header->compressed_image_size);

    // u32 bytew = header->width * 4;
    // u32 msg_pos = (header->file_size) - bytew - bytew - bytew + 8;
    // unsigned char msg_len = file_content.data[file_content.size - bytew + 28] + file_content.data[file_content.size - bytew + 28 + 2];
    // // printf("%d\n", msg_len);
    // i8 *msg = (i8*)malloc(msg_len + 1);
    // msg[msg_len] = 0;
    
    // u8 chunk_count = msg_len / 28;
    // u8 remainder_chunk_len = msg_len % 28;
    // i8 *data = file_content.data + msg_pos; // -chunk count cuzwhile doesnt go to 0
    // i8 *msg_tmp = msg;
    // while (chunk_count) {
    //     read_row(data, msg_tmp, 28);
    //     chunk_count--;
    //     data -= bytew;
    //     msg += 28;
    // }
    // if (remainder_chunk_len) {
    //     read_row(data, msg_tmp, remainder_chunk_len);
    // }
    // write(1, msg, msg_len + 1);


    u32 bytew = header->width * 4;
    i8 *root = (i8 *)find_root((u8 *)file_content.data, file_content.size, header->width * 4);
    if (!root) {
        exit(1);
    }
    u8 r1 = root[7*4];
    u8 r2 = root[7*4 + 2];
    u16 msg_len = r1 + r2;
    // u16 msg_len = 128;
    printf("a %d\n", msg_len);
    // printf("msg_len = %d", msg_len);
    i8 *msg = (i8*)malloc(msg_len + 1);
    msg[msg_len] = 0;
    // u32 msg_pos = root - ((header->width * 4) * 2) + 2*4;
    u8 chunk_count = msg_len / 18;
    u8 remainder_chunk_len = msg_len % 18;
    i8 *data = root - (bytew * 2) + 8;
    i8 *msg_tmp = msg;
    i32 j;
    while (chunk_count) {
        j = read_row(data, msg_tmp, 18);
        chunk_count--;
        data -= bytew;
        msg_tmp += 18;
    }
    if (remainder_chunk_len) {
        read_row(data, msg_tmp, remainder_chunk_len);
    }
    write(1, msg, msg_len + 1);
    free(msg);


    return 0;
}

