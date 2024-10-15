#include "pch.h"

__declspec(dllexport) char* p = nullptr;

namespace yyobj_fileread
{
    TEST_CLASS(obj_test_stdio_fread)
    {
    public:
        //static char* p;
        static void test(std::string path, size_t size, size_t count = 1000)
        {
            //std::unique_ptr<char[]> p = std::make_unique<char[]>(size);
            for (int i = 0; i < count; i++)
            {
                FILE* f;
                errno_t error;
                error = fopen_s(&f, (std::string("../../yyobj_test/bigfiles/") + path).c_str(), "rb");
                Assert::IsNotNull(f);
                if (f) {
                    size_t cnt = fread(p, 1, size, f);
                    Assert::AreEqual(cnt, size);
                    fclose(f);
                }
            }
        }
        TEST_METHOD(fread_00_init_pointer)
        {
            p = new char[1024 * 1024 * 1024];
        }
        TEST_METHOD(fread_01_1k)
        {
            test("k_1kb.bin", 1024);
        }
        TEST_METHOD(fread_02_2k)
        {
            test("k_2kb.bin", 1024 * 2);
        }
        TEST_METHOD(fread_03_4k)
        {
            test("k_4kb.bin", 1024 * 4);
        }
        TEST_METHOD(fread_04_8k)
        {
            test("k_8kb.bin", 1024 * 8);
        }
        TEST_METHOD(fread_05_16k)
        {
            test("k_16kb.bin", 1024 * 16);
        }
        TEST_METHOD(fread_06_32k)
        {
            test("k_32kb.bin", 1024 * 32);
        }
        TEST_METHOD(fread_07_64k)
        {
            test("k_64kb.bin", 1024 * 64);
        }
        TEST_METHOD(fread_08_128k)
        {
            test("k_128kb.bin", 1024 * 128);
        }
        TEST_METHOD(fread_09_256k)
        {
            test("k_256kb.bin", 1024 * 256);
        }
        TEST_METHOD(fread_10_512k)
        {
            test("k_512kb.bin", 1024 * 512);
        }
        TEST_METHOD(fread_11_1m)
        {
            test("m_1mb.bin", 1024 * 1024);
        }
        TEST_METHOD(fread_12_2m)
        {
            test("m_2mb.bin", 1024 * 1024 * 2);
        }
        TEST_METHOD(fread_13_4m)
        {
            test("m_4mb.bin", 1024 * 1024 * 4, 10);
        }
        TEST_METHOD(fread_14_8m)
        {
            test("m_8mb.bin", 1024 * 1024 * 8, 10);
        }
        TEST_METHOD(fread_15_16m)
        {
            test("m_16mb.bin", 1024 * 1024 * 16, 10);
        }
        /*
        TEST_METHOD(fread_16_32m)
        {
            test("m_32mb.bin", 1024 * 1024 * 32, 10);
        }
        TEST_METHOD(fread_17_64m)
        {
            test("m_64mb.bin", 1024 * 1024 * 64, 10);
        }
        TEST_METHOD(fread_18_128m)
        {
            test("m_128mb.bin", 1024 * 1024 * 128, 10);
        }
        TEST_METHOD(fread_19_256m)
        {
            test("m_256mb.bin", 1024 * 1024 * 256, 10);
        }
        TEST_METHOD(fread_20_512m)
        {
            test("m_512mb.bin", 1024 * 1024 * 512, 10);
        }
        TEST_METHOD(fread_21_1g)
        {
            test("g_1gb.bin", 1024 * 1024 * 1024, 10);
        }
        */
        TEST_METHOD(fread_22_free_pointer)
        {
            delete[] p;
        }
    };
}
