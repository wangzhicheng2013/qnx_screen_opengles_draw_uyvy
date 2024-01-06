#include "qnx_screen_opengles_render_image.hpp"
#include <fcntl.h>
#include <iostream>
int main() {
    int screen_width = 0, screen_height = 0;
    std::cout << "screen width:";
    std::cin >> screen_width;
    std::cout << "screen height:";
    std::cin >> screen_height;
    QNX_SCREEN_CTX.set_screen_scale(screen_width, screen_height); 
    int error = QNX_SCREEN_CTX.init();
    if (error) {
        LOG_E("qnx screen context init failed:%d", error);
        return -1;
    }
    OPENGLES_RENDER.set_image_scale(screen_width, screen_height);
    OPENGLES_RENDER.set_image_format(); // default UYVY
    int size = OPENGLES_RENDER.get_image_size();
    unsigned char* image = (unsigned char*)malloc(size);
    if (nullptr == image) {
        return -1;
    }
    char path[128] = { 0 };
    std::cout << "image path:";
    std::cin >> path;
	FILE *fp = fopen(path, "rb");
	if (nullptr == fp) {
        LOG_E("%s open failed!", path);
        return -1;
    } 
    fread(image, size, 1, fp);
    fclose(fp);
    OPENGLES_RENDER.render_image(image);
    sleep(5);
    free(image);
    
    return 0;
}