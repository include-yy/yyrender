#include "pch.h"
#include "ObjWave.h"

namespace yyobj_objwave
{
    TEST_CLASS(obj_test_parse)
    {
    public:
        using errc = yyobj::yyobj_errc;
        static int toInt(yyobj::yyobj_errc v)
        {
            return static_cast<int>(v);
        }
        static void assignV3(float* arr, float x, float y, float z)
        {
            arr[0] = x;
            arr[1] = y;
            arr[2] = z;
        }
        static bool floatEqual(float* f1, float* f2, int len)
        {
            for (int i = 0; i < len; i++)
            {
                if (f1[i] != f2[i])
                    return false;
            }
            return true;
        }
        TEST_METHOD(obj_01_parse_vertex)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            /* normal situation */
            Cstr t4[] = {
                "0.e+4 01. .01\n",
                "12.34 .0 1e-3\n",
                "1 2 3\n",
                " 1.5 -2 3.0 \n",
                "     0.3        -1.5e2    0\n",
                "6.699907 90.489624 -7.897428\n",
                "9.174744 89.922295 -7.233001\n",
                "-0.000000 92.666771 -7.046364\n",
                "1.976214 91.759537 -7.835010\n",
                "11.347529 92.189377 -0.610389\n",
                "10.461395 93.038727 1.370051\n",
                "6.675225 88.674660 7.693306\n",
                "4.537068 87.699539 8.174126\n",
                "10.265428 92.579330 3.424031\n",
                "8.432014 89.867264 6.715698\n",
                "9.680240 91.419716 5.352330\n" };
            float t4_nums[][3] = {
                {0, 1.0, 0.01},
                {12.34, 0.0, 0.001},
                {1.0, 2.0, 3.0},
                {1.5, -2.0, 3.0},
                {0.3, -150, 0},
                {6.699907, 90.489624, -7.897428},
                {9.174744, 89.922295, -7.233001},
                {-0.000000, 92.666771, -7.046364},
                {1.976214, 91.759537, -7.835010},
                {11.347529, 92.189377, -0.610389},
                {10.461395, 93.038727, 1.370051},
                {6.675225, 88.674660, 7.693306},
                {4.537068, 87.699539, 8.174126},
                {10.265428, 92.579330, 3.424031},
                {8.432014, 89.867264, 6.715698},
                {9.680240, 91.419716, 5.352330}
            };
            for (int i = 0; i < _countof(t4); i++)
            {
                yyobj::parse_vertex(ctx, t4[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::IsTrue(floatEqual(ctx.mesh->positions.data() + 3 * i,
                    t4_nums[i], 3));
            }
            offset += 3 * _countof(t4);
            /* deal with extra color */
            Assert::AreEqual(0, static_cast<int>(ctx.mesh->colors.size()));
            Cstr t8[] = {
                "1 2 3 4 5 6\n",
                "2 3 4 5 6 7\n",
                "3 4 5 6 7 8\n"
            };
            float t8_nums[][6] = {
                {1, 2, 3, 4, 5, 6},
                {2, 3, 4, 5, 6, 7},
                {3, 4, 5, 6, 7, 8}
            };
            for (int i = 0; i < _countof(t8); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_vertex(ctx, t8[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(ctx.mesh->colors.size(), ctx.mesh->positions.size());
                Assert::IsTrue(floatEqual(ctx.mesh->positions.data() + offset + i * 3,
                    t8_nums[i], 3));
                Assert::IsTrue(floatEqual(ctx.mesh->colors.data() + offset + i * 3,
                    t8_nums[i] + 3, 3));
            }
            for (int i = 0; i < offset; i++)
            {
                Assert::AreEqual(1.0f, ctx.mesh->colors[i]);
            }
            offset += 3 * _countof(t8);
            /* incorrect number of position */
            Cstr t6[] = {
                "\n",                            // 0
                "1.0\n",                         // 1
                "1.0 2.0\n",                     // 2
                "1.0 2.0 3.0 4.0\n",             // 4
                "1.0 2.0 3.0 4.0 5.0\n",         // 5
                "1.0 2.0 3.0 4.0 5.0 6.0 7.0\n", // 7
                "1 2 3 4 5 6 7 8\n"              // 8
                "1 2 3 4 5 6 7 8 9\n",           // 9
                "1 2 3 4 5 6 7 8 9 0\n"          // 10
            };
            for (int i = 0; i < _countof(t6); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_vertex(ctx, t6[i], err);
                Assert::AreEqual(toInt(errc::ParseFloatError), err.code.value());
                if (i >= 3)
                {
                    Assert::AreEqual(offset + (i - 2) * 3 + 3, ctx.mesh->positions.size());
                }
                if (i >= 5)
                {
                    Assert::AreEqual(offset + (i - 2) * 3 + 3, ctx.mesh->colors.size());
                }
            }
            /* incorrect separator */
            Cstr t5[] = {
                "1.0, 2.0, 3.0\n",
                "1.0 & 3.0 & 4.0\n",
                "-114514 * 223344 @ 13\n",
                "1.0/2.0/3.0\n",
                "3.0~4.0~5.0\n",
                "1.0+2.0=3.0\n",
                "(1.0) (2.0) (3.0)\n",
                "1.0 %  2.0 % 3.0\n",
                "1.0 2.0 3.0\n",
                "1.0 2.0 3.0\n",
                "1.0 2.0 3.0\n",
                "1.0 2.0 3.0\n",
                "1 2 3\n",
                "1 2 3\n",
                "1 2 3\n",
                "1　2　3\n",
                "1 2 3\n",
                "1 2 3\n",
                "1 2 3\n",
                "1 2 3\n",
            };
            for (int i = 0; i < _countof(t5); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_vertex(ctx, t5[i], err);
                Assert::AreEqual(toInt(errc::ParseFloatError), err.code.value());
            }
            /* Arbitary chaos input */
            Cstr t7[] = {
                "",
                "Make Emacs great again!\n",
                "Do not go gentle into that good night\n",
                "秋坟鬼唱鲍家诗，恨血千年土中碧\n",
                "芭蕉野分して盥に雨を聞く夜かな\n",
                "Je pense, donc je suis\n",
                "Add PROBLEMS entry for bug#72517\n",
                "https://www.pixiv.net/artworks/121248164\n",
                "你说的对，但是\n",
                "To invalidate a GPUObjectBase object\n"
            };
            for (int i = 0; i < _countof(t7); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_vertex(ctx, t7[i], err);
                Assert::AreEqual(toInt(errc::ParseFloatError), err.code.value());
            }
        }
        TEST_METHOD(obj_02_parse_texcoord)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            /* Noraml uv parse */
            Cstr t1[] = {
                "1 2\n",
                "1 -1\n",
                "3 10\n",
                "0 0\n",
                "0.5 0.5\n",
                "0.751769 0.705255\n",
                "0.736800 0.693579\n",
                "0.752068 0.698262\n",
                "0.593438 0.698262\n",
                "0.608708 0.693579\n",
                "0.593735 0.705255\n",
                "0.953785 0.578535\n",
                "0.953678 0.573185\n",
                "0.973547 0.573185\n",
                "0.973547 0.578535\n",
                "0.793243 0.675955\n",
                "0.769764 0.675206\n",
                "0.770895 0.651329\n",
                "0.794911 0.653434\n",
                "0.845194 0.578739\n",
                "0.823242 0.578739\n"
            };
            float t1_nums[][2] = {
                {1, 2},
                {1, -1},
                {3, 10},
                {0, 0},
                {0.5, 0.5},
                {0.751769, 0.705255},
                {0.736800, 0.693579},
                {0.752068, 0.698262},
                {0.593438, 0.698262},
                {0.608708, 0.693579},
                {0.593735, 0.705255},
                {0.953785, 0.578535},
                {0.953678, 0.573185},
                {0.973547, 0.573185},
                {0.973547, 0.578535},
                {0.793243, 0.675955},
                {0.769764, 0.675206},
                {0.770895, 0.651329},
                {0.794911, 0.653434},
                {0.845194, 0.578739},
                {0.823242, 0.578739}
            };
            for (int i = 0; i < _countof(t1); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_texcoord(ctx, t1[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::IsTrue(floatEqual(ctx.mesh->texcoords.data() + 2 * i,
                    t1_nums[i], 2));
            }
            /* too few or too many floats */
            Cstr t2[] = {
                "1.0\n",
                //"2.0 3.0 4.0\n",
                //"1 1 1 1\n",
                //"1 1 1 1 1 1\n",
                //"1 1 11 1 1 1 1 11 1 1 1\n"
            };
            for (int i = 0; i < _countof(t2); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_texcoord(ctx, t2[i], err);
                Assert::AreEqual(toInt(errc::ParseFloatError), err.code.value());
            }
        }
        TEST_METHOD(obj_03_parse_normal)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            /* Noraml `normal' parse */
            Cstr t1[] = {
                "0.681957 0.424452 0.595630\n",
                "0.413298 0.294375 0.861701\n",
                "0.618243 0.359707 0.698847\n",
                "-0.618243 0.359707 0.698847\n",
                "-0.413298 0.294375 0.861701\n",
                "-0.681957 0.424452 0.595630\n",
                "-0.041684 0.415665 -0.908562\n",
                "-0.010408 0.110864 -0.993781\n",
                "-0.000000 0.191197 -0.981552\n",
                "-0.000000 0.432156 -0.901799\n",
                "0.819020 0.491534 0.295973\n",
                "0.761196 0.382700 0.523566\n",
                "0.753917 0.359509 0.549874\n",
                "0.832838 0.454876 0.315388\n",
                "0.761337 0.558324 -0.329607\n",
                "0.890331 0.437635 -0.125643\n",
                "0.991597 0.100046 -0.082016\n",
                "0.895284 0.230807 -0.381043\n",
                "0.950455 0.057434 0.305511\n",
                "0.880418 0.345445 0.324857\n",
                "-0.138531 0.408393 -0.902233\n",
                "-0.119072 0.237739 -0.964003\n",
                "0.399149 0.475784 -0.783779\n",
                "0.467506 0.173493 -0.866798\n",
                "0.210935 0.135337 -0.968086\n",
                "0.129822 0.434209 -0.891408\n",
                "0.543353 0.493326 -0.679263\n",
                "0.640975 0.213312 -0.737326\n",
                "-0.114468 0.409409 -0.905142\n",
                "-0.097893 0.419241 -0.902582\n",
                "-0.000000 0.425717 -0.904856\n",
                "-0.000000 0.419161 -0.907912\n",
                "-0.075218 0.379606 -0.922086\n"
            };
            float t1_nums[][3] = {
                {0.681957, 0.424452, 0.595630},
                {0.413298, 0.294375, 0.861701},
                {0.618243, 0.359707, 0.698847},
                {-0.618243, 0.359707, 0.698847},
                {-0.413298, 0.294375, 0.861701},
                {-0.681957, 0.424452, 0.595630},
                {-0.041684, 0.415665, -0.908562},
                {-0.010408, 0.110864, -0.993781},
                {-0.000000, 0.191197, -0.981552},
                {-0.000000, 0.432156, -0.901799},
                {0.819020, 0.491534, 0.295973},
                {0.761196, 0.382700, 0.523566},
                {0.753917, 0.359509, 0.549874},
                {0.832838, 0.454876, 0.315388},
                {0.761337, 0.558324, -0.329607},
                {0.890331, 0.437635, -0.125643},
                {0.991597, 0.100046, -0.082016},
                {0.895284, 0.230807, -0.381043},
                {0.950455, 0.057434, 0.305511},
                {0.880418, 0.345445, 0.324857},
                {-0.138531, 0.408393, -0.902233},
                {-0.119072, 0.237739, -0.964003},
                {0.399149, 0.475784, -0.783779},
                {0.467506, 0.173493, -0.866798},
                {0.210935, 0.135337, -0.968086},
                {0.129822, 0.434209, -0.891408},
                {0.543353, 0.493326, -0.679263},
                {0.640975, 0.213312, -0.737326},
                {-0.114468, 0.409409, -0.905142},
                {-0.097893, 0.419241, -0.902582},
                {-0.000000, 0.425717, -0.904856},
                {-0.000000, 0.419161, -0.907912},
                {-0.075218, 0.379606, -0.922086}
            };
            for (int i = 0; i < _countof(t1); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_normal(ctx, t1[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::IsTrue(floatEqual(ctx.mesh->normals.data() + 3 * i,
                    t1_nums[i], 3));
            }
            /* too few or too many floats */
            Cstr t2[] = {
                "1.0\n",
                "1 1 1 1\n",
                "1 1 1 1 1 1\n",
                "1 1 11 1 1 1 1 11 1 1 1\n"
            };
            for (int i = 0; i < _countof(t2); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_normal(ctx, t2[i], err);
                Assert::AreEqual(toInt(errc::ParseFloatError), err.code.value());
            }
        }
        TEST_METHOD(obj_04_parse_face)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            size_t offset_index{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            /* parse (f v v v ...) */
            Cstr t1[] = {
                "1 2 3\n",
                "4 5 6\n",
                "7 8 9\n",
                "10 11 12\n",
                "1 2 3 4\n",
                "1 2 3 4 5\n",
                "1 2 3 4 5 6 7\n"
            };
            unsigned int t1_nums[] = { 3, 3, 3, 3, 4, 5, 7 };
            unsigned int t1_nums_pos[] = {
                1, 2, 3,
                4, 5, 6,
                7, 8, 9,
                10, 11, 12,
                1, 2, 3, 4,
                1, 2, 3, 4, 5,
                1, 2, 3, 4, 5, 6, 7 };
            size_t t1_offset_index = offset_index;
            for (int i = 0; i < _countof(t1); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_face(ctx, t1[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(ctx.mesh->face_vertices[i], t1_nums[i]);
                for (int j = offset_index; j < offset_index + t1_nums[i]; j++)
                {
                    Assert::AreEqual(ctx.mesh->indices[j].n, 0u);
                    Assert::AreEqual(ctx.mesh->indices[j].t, 0u);
                }
                offset_index += t1_nums[i];
            }
            Assert::AreEqual(_countof(t1), ctx.mesh->face_vertices.size());
            for (int i = t1_offset_index; i < offset_index; i++)
            {
                Assert::AreEqual(ctx.mesh->indices[i].p, t1_nums_pos[i]);
            }
            offset += _countof(t1);
            /* parse (f v/t v/t v/t ...) */
            Cstr t2[] = {
                "1/2 3/4 5/6\n",
                "9/8 7/6 5/4\n",
                "4/4 4/4 4/4\n",
                "1/2 3/4 5/6 7/8\n",
                "1/1 1/1 1/1 1/1 1/1 1/1\n"
            };
            unsigned int t2_offset_index = offset_index;
            unsigned int t2_nums[] = { 3, 3, 3, 4, 6 };
            unsigned int t2_nums_pos[] = {
                1, 3, 5,
                9, 7, 5,
                4, 4, 4,
                1, 3, 5, 7,
                1, 1, 1, 1, 1, 1, 1
            };
            unsigned int t2_nums_tex[] = {
                2, 4, 6,
                8, 6, 4,
                4, 4, 4,
                2, 4, 6, 8,
                1, 1, 1, 1, 1, 1, 1
            };
            for (int i = 0; i < _countof(t2); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_face(ctx, t2[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(ctx.mesh->face_vertices[i + offset], t2_nums[i]);
                for (int j = offset_index; j < offset_index + t2_nums[i]; j++)
                {
                    Assert::AreEqual(ctx.mesh->indices[j].n, 0u);
                }
                offset_index += t2_nums[i];
            }
            Assert::AreEqual(_countof(t2) + offset, ctx.mesh->face_vertices.size());
            for (int i = t2_offset_index; i < offset_index; i++)
            {
                Assert::AreEqual(ctx.mesh->indices[i].p, t2_nums_pos[i - t2_offset_index]);
                Assert::AreEqual(ctx.mesh->indices[i].t, t2_nums_tex[i - t2_offset_index]);
            }
            offset += _countof(t2);
            /* parse (f v//n v//n v//n ... */
            Cstr t3[] = {
                "1//2 3//4 5//6\n",
                "9//8 7//6 5//4\n",
                "4//4 4//4 4//4\n",
                "1//2 3//4 5//6 7//8\n",
                "1//1 1//1 1//1 1//1 1//1 1//1\n"
            };
            unsigned int t3_offset_index = offset_index;
            unsigned int t3_nums[] = { 3, 3, 3, 4, 6 };
            unsigned int t3_nums_pos[] = {
                1, 3, 5,
                9, 7, 5,
                4, 4, 4,
                1, 3, 5, 7,
                1, 1, 1, 1, 1, 1, 1
            };
            unsigned int t3_nums_nor[] = {
                2, 4, 6,
                8, 6, 4,
                4, 4, 4,
                2, 4, 6, 8,
                1, 1, 1, 1, 1, 1, 1
            };
            for (int i = 0; i < _countof(t3); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_face(ctx, t3[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(ctx.mesh->face_vertices[i + offset], t3_nums[i]);
                for (int j = offset_index; j < offset_index + t3_nums[i]; j++)
                {
                    Assert::AreEqual(ctx.mesh->indices[j].t, 0u);
                }
                offset_index += t3_nums[i];
            }
            Assert::AreEqual(_countof(t3) + offset, ctx.mesh->face_vertices.size());
            for (int i = t3_offset_index; i < offset_index; i++)
            {
                Assert::AreEqual(ctx.mesh->indices[i].p, t3_nums_pos[i - t3_offset_index]);
                Assert::AreEqual(ctx.mesh->indices[i].n, t3_nums_nor[i - t3_offset_index]);
            }
            offset += _countof(t3);
            /* parse (f p/t/n p/t/n p/t/n ...*/
            Cstr t4[] = {
                "17/1/1 16/2/2 157/3/3\n",
                "290/4/4 31/5/5 32/6/6\n",
                "61/7/7 47/8/8 34/9/9\n",
                "41/10/10 61/7/7 34/9/9\n",
                "49/11/11 62/12/12 63/13/13\n",
                "53/14/14 49/11/11 63/13/13\n",
                "59/15/15 82/16/16 83/17/17\n",
                "46/18/18 59/15/15 83/17/17\n",
                "45/19/19 83/17/17 82/16/16\n",
                "60/20/20 45/19/19 82/16/16\n",
                "1/1/1 2/2/2 3/3/3 4/4/4\n",
                "1/1/1 2/2/2 3/3/3 4/4/4 5/5/5\n",
            };
            unsigned int t4_offset_index = offset_index;
            unsigned int t4_nums[] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5 };
            unsigned int t4_nums_pos[] = {
                17, 16, 157,
                290, 31, 32,
                61, 47, 34,
                41, 61, 34,
                49, 62, 63,
                53, 49, 63,
                59, 82, 83,
                46, 59, 83,
                45, 83, 82,
                60, 45, 82,
                1, 2, 3, 4,
                1, 2, 3, 4, 5
            };
            unsigned int t4_nums_tex[] = {
                1, 2, 3,
                4, 5, 6,
                7, 8, 9,
                10, 7, 9,
                11, 12, 13,
                14, 11, 13,
                15, 16, 17,
                18, 15, 17,
                19, 17, 16,
                20, 19, 16,
                1, 2, 3, 4,
                1, 2, 3, 4, 5
            };
            unsigned int t4_nums_nor[] = {
                1, 2, 3,
                4, 5, 6,
                7, 8, 9,
                10, 7, 9,
                11, 12, 13,
                14, 11, 13,
                15, 16, 17,
                18, 15, 17,
                19, 17, 16,
                20, 19, 16,
                1, 2, 3, 4,
                1, 2, 3, 4, 5
            };
            for (int i = 0; i < _countof(t4); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_face(ctx, t4[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(ctx.mesh->face_vertices[i + offset], t4_nums[i]);
                offset_index += t4_nums[i];
            }
            Assert::AreEqual(_countof(t4) + offset, ctx.mesh->face_vertices.size());
            for (int i = t4_offset_index; i < offset_index; i++)
            {
                Assert::AreEqual(ctx.mesh->indices[i].p, t4_nums_pos[i - t4_offset_index]);
                Assert::AreEqual(ctx.mesh->indices[i].n, t4_nums_nor[i - t4_offset_index]);
            }
            offset += _countof(t4);
            /* negative index */
            Cstr t5 = "-1/-1/-4 -5/-1/-4 -1/-9/-1\n";
            unsigned int t5_nums_pos[] = { -1, -5, -1 };
            unsigned int t5_nums_tex[] = { -1, -1, -9 };
            unsigned int t5_nums_nor[] = { -4, -4, -1 };
            err.code = yyobj::make_error_code(errc::Success);
            yyobj::parse_face(ctx, t5, err);
            Assert::AreEqual(toInt(errc::Success), err.code.value());
            Assert::AreEqual(1 + offset, ctx.mesh->face_vertices.size());
            for (int i = 0; i < 3; i++)
            {
                Assert::AreEqual(ctx.mesh->indices[i + offset_index].p, t5_nums_pos[i]);
                Assert::AreEqual(ctx.mesh->indices[i + offset_index].t, t5_nums_tex[i]);
                Assert::AreEqual(ctx.mesh->indices[i + offset_index].n, t5_nums_nor[i]);
            }
            offset_index += 3;
            offset += 1;
            /* Check group/object/mtl's face_count */
            Assert::AreEqual(static_cast<unsigned int>(offset), ctx.group.face_count);
            Assert::AreEqual(static_cast<unsigned int>(offset), ctx.object.face_count);
            Assert::AreEqual(static_cast<unsigned int>(offset), ctx.mtl.face_count);
            /* vertex too few or too many */
            std::string t6_256 = "1 ";
            for (int i = 0; i < 8; i++)
            {
                t6_256 = t6_256 + t6_256;
            }
            t6_256 += '\n';
            Cstr t6[] = {
                "1 2\n",
                t6_256.c_str()
            };
            for (int i = 0; i < _countof(t6); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_face(ctx, t6[i], err);
                Assert::AreEqual(toInt(errc::ParseArityMismatchError), err.code.value());
            }
            /* test type mismatch (f v v/t) (f v v//n) (f v v/t/n) ...*/
            Cstr t7[] = {
                "1 2/3 4/5/6\n",
                "2//3 3//4 4/5/6\n",
                "1/2/3 3/2/1 3//1 3/1\n",
                "1 1//2 2/3\n",
                "1 1/2 1//3 1/2/3\n"
            };
            for (int i = 0; i < _countof(t7); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_face(ctx, t7[i], err);
                Assert::AreEqual(toInt(errc::ParseIndexError), err.code.value());
            }
        }
        TEST_METHOD(obj_05_parse_object)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            Cstr t1[] = {
                "abc\n",
                "hello world \n",
                "emacs_skin\n",
                "  hoo \n",
                "这是一个中文名称的对象\n",
                "eng_name\n",
                "これは日本の名前です\n"
            };
            std::vector<std::string> t1_res = {
                "abc",
                "hello world",
                "emacs_skin",
                "hoo",
                "这是一个中文名称的对象",
                "eng_name",
                "これは日本の名前です"
            };
            for (int i = 0; i < _countof(t1); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_object(ctx, t1[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(t1_res[i], ctx.object.name);
            }
            Assert::AreEqual(static_cast<size_t>(6), ctx.mesh->objects.size());
            Cstr t2 = "1 2 3\n";
            err.code = yyobj::make_error_code(errc::Success);
            yyobj::parse_face(ctx, t2, err);
            Assert::AreEqual(toInt(errc::Success), err.code.value());
            err.code = yyobj::make_error_code(errc::Success);
            Cstr t2_str = "abc\n";
            yyobj::parse_object(ctx, t2_str, err);
            Assert::AreEqual(toInt(errc::Success), err.code.value());
            Assert::AreEqual(static_cast<size_t>(7), ctx.mesh->objects.size());
            Assert::AreEqual(std::string("これは日本の名前です"), ctx.mesh->objects[6].name);
            Assert::AreEqual(0u, ctx.mesh->objects[6].index_offset);
            Assert::AreEqual(0u, ctx.mesh->objects[6].face_offset);
            Assert::AreEqual(1u, ctx.mesh->objects[6].face_count);
            /* empty name test */
            Cstr t3[] = {
                "\n",
                " \n",
                "  \n",
                "   \n",
                "    \n",
                "     \n",
                "      \n",
                "\t\n",
                "\t\t\n"
            };
            for (int i = 0; i < _countof(t3); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_object(ctx, t3[i], err);
                Assert::AreEqual(toInt(errc::ObjInvalidKeywordError), err.code.value());
            }
        }
        TEST_METHOD(obj_06_parse_group)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            Cstr t1[] = {
                "abc\n",
                "hello world \n",
                "emacs_skin\n",
                "  hoo \n",
                "这是一个中文名称的对象\n",
                "eng_name\n",
                "これは日本の名前です\n"
            };
            std::vector<std::string> t1_res = {
                "abc",
                "hello world",
                "emacs_skin",
                "hoo",
                "这是一个中文名称的对象",
                "eng_name",
                "これは日本の名前です"
            };
            for (int i = 0; i < _countof(t1); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_group(ctx, t1[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(t1_res[i], ctx.group.name);
            }
            Assert::AreEqual(static_cast<size_t>(6), ctx.mesh->groups.size());
            Cstr t2 = "1 2 3\n";
            err.code = yyobj::make_error_code(errc::Success);
            yyobj::parse_face(ctx, t2, err);
            Assert::AreEqual(toInt(errc::Success), err.code.value());
            err.code = yyobj::make_error_code(errc::Success);
            Cstr t2_str = "abc\n";
            yyobj::parse_group(ctx, t2_str, err);
            Assert::AreEqual(toInt(errc::Success), err.code.value());
            Assert::AreEqual(static_cast<size_t>(7), ctx.mesh->groups.size());
            Assert::AreEqual(std::string("これは日本の名前です"), ctx.mesh->groups[6].name);
            Assert::AreEqual(0u, ctx.mesh->groups[6].index_offset);
            Assert::AreEqual(0u, ctx.mesh->groups[6].face_offset);
            Assert::AreEqual(1u, ctx.mesh->groups[6].face_count);
            /* empty name test */
            Cstr t3[] = {
                "\n",
                " \n",
                "  \n",
                "   \n",
                "    \n",
                "     \n",
                "      \n",
                "\t\n",
                "\t\t\n"
            };
            for (int i = 0; i < _countof(t3); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_group(ctx, t3[i], err);
                Assert::AreEqual(toInt(errc::ObjInvalidKeywordError), err.code.value());
            }
        }
        TEST_METHOD(obj_07_parse_usemtl)
        {
            using Cstr = const char*;
            yyobj::Context ctx{};
            yyobj::Error err{};
            size_t offset{};
            ctx.mesh = std::make_unique<yyobj::Mesh>();
            /* empty name test */
            Cstr t3[] = {
                "\n",
                " \n",
                "  \n",
                "   \n",
                "    \n",
                "     \n",
                "      \n",
                "\t\n",
                "\t\t\n"
            };
            for (int i = 0; i < _countof(t3); i++)
            {
                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_usemtl(ctx, t3[i], err);
                Assert::AreEqual(toInt(errc::ObjInvalidKeywordError), err.code.value());
            }
            // Implementation changed, take a looser rule
            /* material not found */
            //Cstr t4[] = {
            //    "name1",
            //    "name2",
            //    "name3"
            //};
            //for (int i = 0; i < _countof(t4); i++)
            //{
            //    err.code = yyobj::make_error_code(errc::Success);
            //    yyobj::parse_usemtl(ctx, t4[i], err);
            //    Assert::AreEqual(toInt(errc::MaterialNotFoundError), err.code.value());
            //}
            /* test mock materials */
            yyobj::Material tmpMtl{};
            tmpMtl.name = "abc";
            ctx.mesh->materials.push_back(tmpMtl);
            tmpMtl.name = "def";
            ctx.mesh->materials.push_back(tmpMtl);
            tmpMtl.name = "ghi";
            ctx.mesh->materials.push_back(tmpMtl);
            Cstr t5[] = {
                "def\n",
                "ghi\n",
                "abc\n",
            };
            unsigned int t5_idx[] = { 1, 2, 0 };
            for (int i = 0; i < _countof(t5); i++)
            {
                //Cstr tf = "1 2 3\n";
                //err.code = yyobj::make_error_code(errc::Success);
                //yyobj::parse_face(ctx, tf, err);
                //Assert::AreEqual(toInt(errc::Success), err.code.value());

                err.code = yyobj::make_error_code(errc::Success);
                yyobj::parse_usemtl(ctx, t5[i], err);
                Assert::AreEqual(toInt(errc::Success), err.code.value());
                Assert::AreEqual(t5_idx[i], ctx.mtl.index);
            }
        }
    };
}
