#include "scpch.h"
#include <glad/glad.h>
#include "Logger.h"
#include <GLFW/glfw3.h>
#include <Shader.h>
#include "VideoStreamClient.h";



using namespace SC;

Logger m_Logger;

#define INBUF_SIZE 1024


unsigned int texture1 = 0;
unsigned int texture2 = 0;


void generateTexture(unsigned char* pixels)
{
    glGenTextures(1, &texture1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}

//static void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const char* filename, unsigned char* pixels)
//{
//    struct SwsContext* sws_ctx = NULL;
//    char buf[1024];
//    int ret;
//    int sts;
//
//    ret = avcodec_send_packet(dec_ctx, pkt);
//    if (ret < 0)
//    {
//        fprintf(stderr, "Error sending a packet for decoding\n");
//        exit(1);
//    }
//
//    //Create SWS Context for converting from decode pixel format (like YUV420) to RGB
//    ////////////////////////////////////////////////////////////////////////////
//    sws_ctx = sws_getContext(dec_ctx->width,
//        dec_ctx->height,
//        dec_ctx->pix_fmt,
//        dec_ctx->width,
//        dec_ctx->height,
//        AV_PIX_FMT_RGB24,
//        SWS_BICUBIC,
//        NULL,
//        NULL,
//        NULL);
//
//    if (sws_ctx == nullptr)
//    {
//        return;  //Error!
//    }
//    ////////////////////////////////////////////////////////////////////////////
//
//
//    //Allocate frame for storing image converted to RGB.
//    ////////////////////////////////////////////////////////////////////////////
//    AVFrame* pRGBFrame = av_frame_alloc();
//
//    pRGBFrame->format = AV_PIX_FMT_RGB24;
//    pRGBFrame->width = dec_ctx->width;
//    pRGBFrame->height = dec_ctx->height;
//
//    sts = av_frame_get_buffer(pRGBFrame, 0);
//
//    if (sts < 0)
//    {
//        return;  //Error!
//    }
//    ////////////////////////////////////////////////////////////////////////////
//
//
//    while (ret >= 0)
//    {
//        ret = avcodec_receive_frame(dec_ctx, frame);
//        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//        {
//            return;
//        }
//        else if (ret < 0)
//        {
//            fprintf(stderr, "Error during decoding\n");
//            exit(1);
//        }
//
//        printf("saving frame %3d\n", dec_ctx->frame_num);
//        fflush(stdout);
//
//        /* the picture is allocated by the decoder. no need to
//           free it */
//           //snprintf(buf, sizeof(buf), "%s_%03d.pgm", filename, dec_ctx->frame_number);
//           //pgm_save(frame->data[0], frame->linesize[0],
//           //    frame->width, frame->height, buf);
//
//           //Convert from input format (e.g YUV420) to RGB and save to PPM:
//           ////////////////////////////////////////////////////////////////////////////
//        sts = sws_scale(sws_ctx,                //struct SwsContext* c,
//            frame->data,            //const uint8_t* const srcSlice[],
//            frame->linesize,        //const int srcStride[],
//            0,                      //int srcSliceY, 
//            frame->height,          //int srcSliceH,
//            pRGBFrame->data,        //uint8_t* const dst[], 
//            pRGBFrame->linesize);   //const int dstStride[]);
//
//        if (sts != frame->height)
//        {
//            return;  //Error!
//        }
//
//        snprintf(buf, sizeof(buf), "%s_%03d.ppm", filename, dec_ctx->frame_num);
//        memcpy(pixels, pRGBFrame->data[0], 1920 * 1080 * 3);
//        //ppm_save(pRGBFrame->data[0], pRGBFrame->linesize[0], pRGBFrame->width, pRGBFrame->height, buf);
//        ////////////////////////////////////////////////////////////////////////////
//    }
//
//    //Free
//    sws_freeContext(sws_ctx);
//    av_frame_free(&pRGBFrame);
//}

//static void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt,
//    const char* filename, unsigned char* pixels)
//{
//    char buf[1024];
//    int ret;
//
//    ret = avcodec_send_packet(dec_ctx, pkt);
//    if (ret < 0) {
//        fprintf(stderr, "Error sending a packet for decoding\n");
//        exit(1);
//    }
//
//    while (ret >= 0) {
//        ret = avcodec_receive_frame(dec_ctx, frame);
//        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//            return;
//        else if (ret < 0) {
//            fprintf(stderr, "Error during decoding\n");
//            exit(1);
//        }
//
//        /* the picture is allocated by the decoder. no need to
//           free it */
//        m_Logger.Info("Size {0}", sizeof(*(frame->data[0])));
//        memcpy(pixels, frame->data, sizeof(*(frame->data)));
//        //pgm_save(frame->data[0], frame->linesize[0],
//        //    frame->width, frame->height, buf);
//
//        //av_frame_unref(frame);    // <--- I think we need it.
//    }
//
//    //av_packet_unref(pkt);  // <--- I think we need it.
//}

int main(int argc, char** argv)
{


    m_Logger = Logger();
    m_Logger.CreateConsole();
    m_Logger.Info("Test");

    //Init GLW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, 1920, 1080);






    unsigned char* pixels = new unsigned char[1920 * 1080 * 3];

    memset(pixels, 1920 * 1080 * 3, 0);

    float vertices[] = {
        // positions          // colors           // texture1 coords
         1.f,  1.f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
         1.f, -1.f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
        -1.f, -1.f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
        -1.f, 1.f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
    };
    unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture1 coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);



    Shader ourShader("Include/Shaders/texture.vs", "Include/Shaders/texture.fs");


    generateTexture(0);

    const char* filename = "TestClient.h265";
    FILE* f;
    //f = fopen(filename, "rb");
    //if (!f)
    //{
    //    fprintf(stderr, "Could not open %s\n", filename);
    //    exit(1);
    //}

    Decoder d = Decoder();
    d.InitializeDecoder();
    
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t dataSize;

    //uint8_t* pixels;

    //while (!feof(f))
    //{
    //    dataSize = fread(inbuf, 1, INBUF_SIZE, f);
    //    if (!dataSize) break;

    //    data = inbuf;

    //    while (dataSize > 0)
    //    {


    //        int len = d.Decode(data, dataSize, pixels);

    //        data += len;
    //        dataSize -= len;
    //    }


    //}



    std::ofstream fpOut("TestClient.h265", std::ios::out | std::ios::binary);

    VideoStreamClient client = VideoStreamClient();
    client.m_Logger = m_Logger;
    client.Initialize(fpOut);
    client.Listen(fpOut);

    int time = 0;
    
    while (!glfwWindowShouldClose(window))
	{
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        ourShader.use();
        ourShader.setInt("texture1", 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //time += glfwGetTime();

        //for (int i = 500; i < 1000; i++)
        //{
        //    for (int j = 500; j < 1000; j++)
        //    {
        //        pixels[3 * (i * 1920 + j) + 0] = time % 255;
        //        pixels[3 * (i * 1920 + j) + 1] = 0;
        //        pixels[3 * (i * 1920 + j) + 2] = 0;
        //    }
        //}





        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1920, 1080, GL_RGB, GL_UNSIGNED_BYTE, client.GetPixels());
		glfwSwapBuffers(window);
		glfwPollEvents();


	}


	glfwTerminate();

    

    //fpOut.close();

    return 0;
}



