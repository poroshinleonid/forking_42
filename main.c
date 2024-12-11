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


// Function to extract and normalize a single color channel
i8 extract_channel(u32 value, u32 mask) {
    if (mask == 0) return 0; // No mask defined for this channel
    int shift = 0;

    // Determine the shift amount by finding the first set bit in the mask
    while (((mask >> shift) & 1) == 0) {
        shift++;
    }

    // Extract the bits and normalize to 8 bits
    uint32_t channel = (value & mask) >> shift;

    // Calculate the number of bits in the channel
    int bits = 0;
    uint32_t temp_mask = mask >> shift;
    while (temp_mask) {
        bits++;
        temp_mask >>= 1;
    }

    // Normalize to 8 bits
    if (bits < 8) {
        channel = (channel << (8 - bits)) | (channel >> (2 * bits - 8));
    } else if (bits > 8) {
        channel = channel >> (bits - 8);
    }

    return (i8)channel;
}



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
    u32 rmask;
    u32 gmask;
    u32 bmask;
    u32 umask;
    
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

void read_row(i8 *src, i8 *dst, u8 real_bytes_to_read) {
    u8 i = 0;
    while (i < real_bytes_to_read) {
        if (i & 3) {
            continue;
        }
        dst[i] = src[i];
        i++;
    }
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
	printf("signature: %.2s\nfile_size: %u\ndata_offset: %u\ninfo_header_size: %u\nwidth: %u\nheight: %u\nplanes: %i\nbit_per_px: %i\ncompression_type: %u\ncompression_size: %u\n", header->signature, header->file_size, header->data_offset, header->info_header_size, header->width, header->height, header->number_of_planes, header->bit_per_pixel, header->compression_type, header->compressed_image_size);
	printf("rmask: %u\ngmask: %u\nbmask: %u\numask: %u\n", header->rmask, header->gmask, header->bmask, header->umask);

    u32 bytew = header->width * 4;
    u32 msg_pos = (header->file_size) - bytew - bytew - bytew + 8;
    unsigned char msg_len = file_content.data[file_content.size - bytew + 28] + file_content.data[file_content.size - bytew + 28 + 2];
    printf("%d\n", msg_len);
    i8 *msg = (i8*)malloc(msg_len + 1);
    msg[msg_len] = 0;
    // int pixels = msg_len / 3;
    // int bytes_leftover = msg_len % 3;
    // int bytes = pixels * 4;
    // bytes += bytes_leftover;

    // unsigned char c;


    // for (u16 i = 0; i <= bytes; i += 4) {
    //     for (u16 j = 0; j < 3; j++){
    //         c = *(file_content.data + msg_pos + i + j);
    //         write(1, &c, 1);
    //     }
    //     // print_px_d(file_content.data + msg_pos + i, header->rmask, header->gmask, header->bmask);
    // }
    
    u8 chunk_count = msg_len / 28;
    u8 remainder_chunk_len = msg_len % 28;
    i8 *data = file_content.data + msg_pos; // -chunk count cuzwhile doesnt go to 0
    i8 *msg_tmp = msg;
    while (chunk_count) {
        read_row(data, msg_tmp, 28);
        chunk_count--;
        data -= bytew;
        msg += 28;
    }
    read_row(data, msg_tmp, remainder_chunk_len);
    write(1, msg, msg_len);

    return 0;
}

