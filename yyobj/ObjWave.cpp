#include "stdafx.h"
#include "ObjWave.h"

/* Error related */
namespace yyobj
{
    Error::operator bool() const noexcept
    {
        return code.value();
    }
    const char* yyobj_error_category::name() const noexcept
    {
        return "yyobj";
    }
    std::string yyobj_error_category::message(int err) const
    {
        switch (static_cast<yyobj_errc>(err))
        {
        case yyobj_errc::Success:
            return "No error.";
        /* errors related to obj file */
        case yyobj_errc::ObjInvalidPathError:
            return "Obj file path not valid.";
        case yyobj_errc::ObjNotFileError:
            return "Obj file path is not a file.";
        case yyobj_errc::ObjInvalidExtError:
            return "Obj file name not ends with OBJ or obj.";
        case yyobj_errc::ObjOpenFileError:
            return "Obj file open failed.";
        case yyobj_errc::ObjAllocBufferError:
            return "Obj file buffer allocate failed.";
        case yyobj_errc::ObjInvalidKeywordError:
            return "Invalid keyword error.";
        case yyobj_errc::ParseFloatError:
            return "Parse float number error.";
        case yyobj_errc::ParseIntError:
            return "Parse integer number error.";
        case yyobj_errc::ParseIndexError:
            return "Parse face index error.";
        case yyobj_errc::MaterialNotFoundError:
            return "Material not found.";
        case yyobj_errc::ParseArityMismatchError:
            return "Arity Mismatch Error.";
        /* errors related to mtl file */
        case yyobj_errc::MaterialFileError:
            return "Material file error.";
        case yyobj_errc::MaterialInvalidPathError:
            return "Material file path not valid.";
        case yyobj_errc::MaterialNotFileError:
            return "Material file path is not a file";
        case yyobj_errc::MaterialInvalidExtError:
            return "Material file name not ends with MTL or mtl.";
        case yyobj_errc::MaterialOpenFileError:
            return "Mtl file open failed";
        case yyobj_errc::MaterialAllocBufferError:
            return "Mtl file buffer allocate failed.";
        case yyobj_errc::MaterialNotSupportError:
            return "Mtl file featrue not supported.";
        case yyobj_errc::MaterialTextureNotFoundError:
            return "Mtl Texture file not found.";
        case yyobj_errc::MaterialParseError:
            return "Material file parse error.";
        /* Mass errors */
        case yyobj_errc::TriangulationError:
            return "Triangulation errror.";
        case yyobj_errc::InternalError:
            return "Internal error.";
        }
        return "Unrecognised error.";
    }

    std::error_code make_error_code(yyobj_errc err)
    {
        static const yyobj_error_category category{};
        return { static_cast<int>(err), category };
    }
}

/// <summary>
/// test whether c is a blank charachter
/// </summary>
/// <param name="c">a char value</param>
/// <returns>true if c is blank char, otherwise false</returns>
static inline bool is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\r');
}

/// <summary>
/// Move pointer to the next place of '\n'
/// </summary>
/// <param name="p"></param>
static inline void skip_line(const char*& p)
{
    while ((*p++) != '\n')
        ;
}

/// <summary>
/// Move pointer to the next place of blank characters
/// </summary>
/// <param name="p"></param>
static inline void skip_whitespace(const char*& p)
{
    while (is_whitespace(*p))
        p++;
}

/// <summary>
/// Move pointer to the next place of a name
/// </summary>
/// <param name="p"></param>
static inline void skip_name(const char*& p)
{
    auto s = p;
    while (*p != '\n')
        p++;
    while (p > s && is_whitespace(*(p - 1)))
        p--;
}

/* .obj file parsing */
namespace yyobj
{
    Result ParseObjFile(std::string pathStr)
    {
        std::error_code err{};
        std::filesystem::path path = pathStr.c_str();
        path = std::filesystem::canonical(path, err);
        /* Not a valid path */
        if (err)
        {
            err = make_error_code(yyobj_errc::ObjInvalidPathError);
            return Result{ nullptr, Error{err, pathStr}};
        }
        /* Not a file path */
        if (!std::filesystem::is_regular_file(path))
        {
            err = make_error_code(yyobj_errc::ObjNotFileError);
            return Result{ nullptr, Error{err, pathStr} };
        }
        /* Not ends with OBJ|obj extension */
        /* Although categoried by filename extension is not good enough ...*/
        if ((!path.has_extension()) ||
            ((path.extension() != ".obj") &&
             (path.extension() != ".OBJ")))
        {
            err = make_error_code(yyobj_errc::ObjInvalidExtError);
            return Result{ nullptr, Error{err, path.string()}};
        }
        FILE* file{};
        errno_t cerr{};
        cerr = fopen_s(&file, path.string().c_str(), "rb");
        /* Open file failed */
        if (cerr)
        {
            err = make_error_code(yyobj_errc::ObjOpenFileError);
            return Result{ nullptr, Error{err, path.string()} };
        }
        /* Create Parsing Context */
        Context ctx{};
        ctx.mesh = std::make_unique<Mesh>();
        ctx.dir = path.parent_path();
        /* NO default group and object */
        //ctx.group.name = "default_group";
        //ctx.object.name = "default_object";

        /* Add dummy position/texcoord/normal/texture */
        /* Since obj's index starts at ONE, not ZERO */
        // position
        ctx.mesh->positions.push_back(0.0f);
        ctx.mesh->positions.push_back(0.0f);
        ctx.mesh->positions.push_back(0.0f);
        // texcoord
        ctx.mesh->texcoords.push_back(0.0f);
        ctx.mesh->texcoords.push_back(0.0f);
        // normal
        ctx.mesh->normals.push_back(0.0f);
        ctx.mesh->normals.push_back(0.0f);
        ctx.mesh->normals.push_back(0.0f);
        // material
        ctx.mesh->textures.push_back(Texture());

        /* Buffer for read from OBJ file */
        /* _aligned_malloc learned form rapidobj library */
        char* buffer = static_cast<char*>(_aligned_malloc(
            2 * BUFFER_SIZE,
            BUFFER_ALIGN));
        /* allcate buffer memory failed */
        if (!buffer)
        {
            err = make_error_code(yyobj_errc::ObjAllocBufferError);
            return Result{ nullptr, Error(err) };
        }
        /* initialzie buffer with zero value */
        std::memset(buffer, 0, 2 * BUFFER_SIZE);
        /* cleanup function */
        auto cleanup = [&file, &buffer]() -> void
        {
            fclose(file);
            _aligned_free(buffer);
        };
        /* buffer range pointers */
        char* start{};
        char* end{};
        char* last{};

        start = buffer;
        Error parse_error{};
        parse_error.code = make_error_code(yyobj_errc::Success);
        parse_error.message = path.string();
        /* parsing loop learned from fast_obj */
        for (;;)
        {
            size_t read = fread(start, 1, BUFFER_SIZE, file);
            /* Finish */
            if (read == 0 && start == buffer)
                break;
            /* Ensure buffer ends in a newline */
            if (read < BUFFER_SIZE)
            {
                if (read == 0 || start[read - 1] != '\n')
                    start[read++] = '\n';
            }
            end = start + read;
            if (end == buffer)
                break;
            /* Find last new line */
            last = end;
            while (last > buffer)
            {
                last--;
                if (*last == '\n')
                    break;
            }
            /* Check there actually is a newline */
            if (*last != '\n')
            {
                break;
            }
            last++;
            parse_buffer(ctx, buffer, last, parse_error);
            /* error occurs during parsing */
            if (parse_error)
            {
                cleanup();
                return { nullptr, parse_error };
            }
            /* move remaining parts to start of buffer */
            size_t bytes = end - last;
            std::memmove(buffer, last, bytes);
            start = buffer + bytes;
        }
        /* do clean up*/
        cleanup();

        /* add the last group, object and material Group to Mesh */
        // if (ctx.object.face_count > 0)
        if (ctx.object.name != "")
            ctx.mesh->objects.push_back(ctx.object);
        //if (ctx.group.face_count > 0)
        if (ctx.group.name != "")
            ctx.mesh->groups.push_back(ctx.group);
        if (ctx.mtl.face_count > 0)
            ctx.mesh->usemtls.push_back(ctx.mtl);

        ctx.mesh->position_count = ctx.mesh->positions.size() / 3;
        ctx.mesh->texcoord_count = ctx.mesh->texcoords.size() / 2;
        ctx.mesh->normal_count = ctx.mesh->normals.size() / 3;
        ctx.mesh->color_count = ctx.mesh->colors.size() / 3;


        /* get results normally */
        return { std::move(ctx.mesh), parse_error };
    }

    void parse_buffer(Context& ctx, const char* ptr, const char* end, Error& err)
    {
        const char* p = ptr;
        const char* line_start{ p };
        while (p != end)
        {
            line_start = p;
            skip_whitespace(p);
            switch (*p)
            {
            /* v, vt and vn*/
            case 'v':
                p++;
                switch (*p++)
                {
                case ' ':
                case '\t':
                    parse_vertex(ctx, p, err);
                    break;
                case 't':
                    parse_texcoord(ctx, p, err);
                    break;
                case 'n':
                    parse_normal(ctx, p, err);
                    break;
                default:
                    goto parse_failed;
                    break;
                }
                break;
            /* face */
            case 'f':
                p++;
                switch (*p++)
                {
                case ' ':
                case '\t':
                    parse_face(ctx, p, err);
                    break;
                default:
                    goto parse_failed;
                    break;
                }
                break;
            /* object */
            case 'o':
                p++;
                switch (*p++)
                {
                case ' ':
                case '\t':
                    parse_object(ctx, p, err);
                    break;
                default:
                    goto parse_failed;
                    break;
                }
                break;
            /* group */
            case 'g':
                p++;
                switch (*p++)
                {
                case ' ':
                case '\t':
                    parse_group(ctx, p, err);
                    break;
                default:
                    goto parse_failed;
                    break;
                }
                break;
            /* material */
            case 'm':
                p++;
                if (p[0] == 't' &&
                    p[1] == 'l' &&
                    p[2] == 'l' &&
                    p[3] == 'i' &&
                    p[4] == 'b' &&
                    is_whitespace(p[5]))
                {
                    p += 5;
                    auto context_line = ctx.line;
                    std::string message = err.message;
                    parse_mtllib(ctx, p, err);
                    if (!err)
                    {
                        ctx.line = context_line;
                        err.message = message;
                    }
                    else
                    {
                        // deal with mtl error separately.
                        goto material_error;
                    }
                }
                else
                {
                    goto parse_failed;
                }
                break;
            /* usemtl */
            case 'u':
                p++;
                if (p[0] == 's' &&
                    p[1] == 'e' &&
                    p[2] == 'm' &&
                    p[3] == 't' &&
                    p[4] == 'l' &&
                    is_whitespace(p[5]))
                {
                    p += 5;
                    parse_usemtl(ctx, p, err);
                }
                else [[unlikely]]
                {
                    goto parse_failed;
                }
                break;
            /* comment */
            case '#':
                break;
            }
            if (err)
            {
                err.line_num = ctx.line;
                auto line_end = line_start;
                skip_line(line_end);
                err.line = std::string(line_start, line_end);
                return;
            }
            skip_line(p);
            ctx.line++;
        }
        /* if we have colors per `v' */
        if (ctx.mesh->colors.size() > 0)
        {
            for (size_t i = ctx.mesh->colors.size(); i < ctx.mesh->positions.size(); i++)
            {
                ctx.mesh->colors.push_back(1.0f);
            }
        }
    material_error:
        return;
    parse_failed:
        if (!err)
            err.code = make_error_code(yyobj_errc::ObjInvalidKeywordError);
        err.line_num = ctx.line;
        auto line_end = line_start;
        skip_line(line_end);
        err.line = std::string(line_start, line_end);
        return;
    }

    void parse_vertex(Context& ctx, const char*& p, Error& err)
    {
        float v{};
        for (size_t i = 0; i < 3; i++)
        {
            skip_whitespace(p);
            auto result = fast_float::from_chars(p, char_nullptr, v);
            if (result.ec != std::errc()) [[unlikely]]
            {
                err.code = make_error_code(yyobj_errc::ParseFloatError);
                return;
            }
            p = result.ptr;
            ctx.mesh->positions.push_back(v);
        }
        skip_whitespace(p);
        /* Support optional color behind position */
        if (*p != '\n') [[unlikely]]
        {
            /* Add missing default colors for ordinary positions */
            for (size_t i = ctx.mesh->colors.size(); i < ctx.mesh->positions.size() - 3; i++)
            {
                ctx.mesh->colors.push_back(1.0f);
            }
            for (size_t i = 0; i < 3; i++)
            {
                skip_whitespace(p);
                auto result = fast_float::from_chars(p, char_nullptr, v);
                if (result.ec != std::errc())
                {
                    err.code = make_error_code(yyobj_errc::ParseFloatError);
                    return;
                }
                p = result.ptr;
                ctx.mesh->colors.push_back(v);
            }
            /* have extra chars even after six vertex data */
            skip_whitespace(p);
            if (*p != '\n')
            {
                err.code = make_error_code(yyobj_errc::ParseFloatError);
                return;
            }
        }
        return;
    }

    void parse_texcoord(Context& ctx, const char*& p, Error& err)
    {
        float v{};
        for (size_t i = 0; i < 2; i++)
        {
            skip_whitespace(p);
            auto result = fast_float::from_chars(p, char_nullptr, v);
            if (result.ec != std::errc()) [[unlikely]]
            {
                err.code = make_error_code(yyobj_errc::ParseFloatError);
                return;
            }
            p = result.ptr;
            ctx.mesh->texcoords.push_back(v);
        }
        // allow additional thrid coord, but we just ignore it.
        //skip_whitespace(p);
        ///* if there are still something after two float */
        //if (*p != '\n')
        //{
        //    err.code = make_error_code(yyobj_errc::ParseFloatError);
        //}
        return;
    }

    void parse_normal(Context& ctx, const char*& p, Error& err)
    {
        float v{};
        for (size_t i = 0; i < 3; i++)
        {
            skip_whitespace(p);
            auto result = fast_float::from_chars(p, char_nullptr, v);
            if (result.ec != std::errc()) [[unlikely]]
            {
                err.code = make_error_code(yyobj_errc::ParseFloatError);
                return;
            }
            p = result.ptr;
            ctx.mesh->normals.push_back(v);
        }
        skip_whitespace(p);
        /* if there are still something after three float */
        if (*p != '\n') [[unlikely]]
        {
            err.code = make_error_code(yyobj_errc::ParseFloatError);
        }
        return;
    }

    void parse_face(Context& ctx, const char*& p, Error& err)
    {
        /* state enumalation */
        enum {cond_INIT, cond_V, cond_VT, cond_VN, cond_VTN};
        /* record 1st element's state: v/vt/vtn/vn */
        unsigned int cond = cond_INIT;
        /* element number counter */
        unsigned int count{};
        VertexIndex idx{};
        int v{}, t{}, n{};

        skip_whitespace(p);
        while (*p != '\n')
        {
            unsigned int iter_cond = cond_INIT;
            v = t = n = 0;
            // parse position index
            auto result = fast_float::from_chars(p, char_nullptr, v);
            if (result.ec != std::errc()) [[unlikely]]
            {
                err.code = make_error_code(yyobj_errc::ParseIntError);
                return;
            }
            p = result.ptr;
            // f v1 v2 v3 ...
            iter_cond = cond_V;
            if (*p == '/')
            {
                p++;
                if (*p != '/')
                {
                    // parse texcoord index
                    result = fast_float::from_chars(p, char_nullptr, t);
                    if (result.ec != std::errc()) [[unlikely]]
                    {
                        err.code = make_error_code(yyobj_errc::ParseIntError);
                        return;
                    }
                    p = result.ptr;
                    // f v/t v/t v/t ...
                    iter_cond = cond_VT;
                }
                else
                {
                    // f v//n v//n v//n
                    iter_cond = cond_VN;
                }

                if (*p == '/')
                {
                    p++;
                    // parse normal index
                    result = fast_float::from_chars(p, char_nullptr, n);
                    if (result.ec != std::errc()) [[unlikely]]
                    {
                        err.code = make_error_code(yyobj_errc::ParseIntError);
                        return;
                    }
                    p = result.ptr;
                    // f v/t/n
                    iter_cond = (iter_cond == cond_VN) ? cond_VN : cond_VTN;
                }
            }
            // initialize first face's condtion
            cond = cond ? cond : iter_cond;
            // type mismatch with first item
            if (cond != iter_cond) [[unlikely]]
            {
                err.code = make_error_code(yyobj_errc::ParseIndexError);
                return;
            }

            if (v < 0)
                idx.p = static_cast<unsigned int>(ctx.mesh->positions.size()) / 3 + v;
            else if (v > 0) [[likely]]
                idx.p = v;
            else
            {
                // position's index can't be ZERO
                err.code = make_error_code(yyobj_errc::ParseIndexError);
                return;
            }

            if (t < 0)
            {
                idx.t = static_cast<unsigned int>(ctx.mesh->texcoords.size()) / 2 + t;
            }
            else [[likely]]
            {
                idx.t = t;
            }

            if (n < 0)
            {
                idx.n = static_cast<unsigned int>(ctx.mesh->normals.size()) / 3 + n;
            }
            else [[likely]]
            {
                idx.n = n;
            }
            ctx.mesh->indices.push_back(idx);
            count++;
            skip_whitespace(p);
        }
        // vertex too few or too many
        if (count < 3 || count > 255) [[unlikely]]
        {
            err.code = make_error_code(yyobj_errc::ParseArityMismatchError);
            return;
        }
        ctx.mesh->face_vertices.push_back(count);
        ctx.mesh->face_materials.push_back(ctx.mtl.index);

        ctx.group.face_count++;
        ctx.object.face_count++;
        ctx.mtl.face_count++;
        return;
    }

    /* push current object to mesh's object array */
    void flush_object(Context& ctx)
    {
        //if (ctx.object.face_count > 0)
        if (ctx.object.name != "")
            ctx.mesh->objects.push_back(ctx.object);
        ctx.object.face_offset = static_cast<unsigned int>(ctx.mesh->face_vertices.size());
        ctx.object.index_offset = static_cast<unsigned int>(ctx.mesh->indices.size());
        ctx.object.face_count = 0;
    }
    void parse_object(Context& ctx, const char*& p, Error& err)
    {
        // push last object ot object list
        flush_object(ctx);
        /* parse start */
        skip_whitespace(p);
        // name part is empty
        if (*p == '\n') [[unlikely]]
        {
            err.code = make_error_code(yyobj_errc::ObjInvalidKeywordError);
            return;
        }
        auto st = p;
        skip_name(p);
        ctx.object.name = std::string(st, p);
        return;
    }
    void flush_group(Context& ctx)
    {
        //if (ctx.group.face_count > 0)
        if (ctx.group.name != "")
            ctx.mesh->groups.push_back(ctx.group);
        ctx.group.face_offset = static_cast<unsigned int>(ctx.mesh->face_vertices.size());
        ctx.group.index_offset = static_cast<unsigned int>(ctx.mesh->indices.size());
        ctx.group.face_count = 0;
    }
    void parse_group(Context& ctx, const char*& p, Error& err)
    {
        /* push current group to mesh's group list */
        flush_group(ctx);
        skip_whitespace(p);
        if (*p == '\n') [[unlikely]]
        {
            err.code = make_error_code(yyobj_errc::ObjInvalidKeywordError);
            return;
        }
        auto st = p;
        skip_name(p);
        ctx.group.name = std::string(st, p);
        return;
    }
    void flush_usemtl(Context& ctx)
    {
        if (ctx.mtl.face_count > 0)
            ctx.mesh->usemtls.push_back(ctx.mtl);
        ctx.mtl.face_offset = static_cast<unsigned int>(ctx.mesh->face_vertices.size());
        ctx.mtl.index_offset = static_cast<unsigned int>(ctx.mesh->indices.size());
        ctx.mtl.face_count = 0;
    }
    void parse_usemtl(Context& ctx, const char*& p, Error& err)
    {
        /* puhs current usemtl to mesh's usemtl list */
        flush_usemtl(ctx);
        skip_whitespace(p);
        /* empty usemtl name */
        if (*p == '\n') [[unlikely]]
        {
            err.code = make_error_code(yyobj_errc::ObjInvalidKeywordError);
            return;
        }
        auto st = p;
        skip_name(p);
        std::string name = std::string(st, p);
        unsigned int idx{ 0 };
        while (idx < ctx.mesh->materials.size())
        {
            if (name == ctx.mesh->materials[idx].name)
                break;
            idx++;
        }
        // Material not found, create a default material
        if (idx == ctx.mesh->materials.size())
        {
            // learned from fast_obj, it seems that use a default one is a better choice...
            // old method: return an error code
            //err.code = make_error_code(yyobj_errc::MaterialNotFoundError);
            //return;
            Material new_mtl{};
            new_mtl.name = name;
            new_mtl.fallback = true;
            ctx.mesh->materials.push_back(std::move(new_mtl));
        }
        ctx.mtl.index = idx;
        return;
    }
}
/* material MTL file parsing */
namespace yyobj
{
    void parse_mtllib(Context& ctx, const char*& ptr, Error& err)
    {
        skip_whitespace(ptr);
        /* Empty mtllib line */
        if (*ptr == '\n')
        {
            err.code = make_error_code(yyobj_errc::ObjInvalidKeywordError);
            return;
        }
        auto st = ptr;
        skip_name(ptr);
        auto ed = ptr;
        // get mtl file path
        std::string mtlPath = std::string(st, ed);
        /* Store mtl file path to err's message member */
        err.message = mtlPath;
        // if file name length less than 4, it must not be a valid mtl file
        // since the possible shortest file is `.mtl'
        if (mtlPath.length() < 4)
        {
            err.code = make_error_code(yyobj_errc::MaterialFileError);
            return;
        }
        // (hack), In Windows, absolute paths always begin with a drive letter.
        // If a path is absolute, the second character in the path will always
        // be a colon `:'.
        std::filesystem::path path;
        if (mtlPath[1] != ':')
        {
            path = ctx.dir / mtlPath;
        }
        else
        {
            path = mtlPath;
        }
        std::error_code errc;
        path = std::filesystem::canonical(path, errc);
        // file not exists
        if (errc)
        {
            err.code = make_error_code(yyobj_errc::MaterialInvalidPathError);
            return;
        }
        // not a file
        if (!std::filesystem::is_regular_file(path))
        {
            err.code = make_error_code(yyobj_errc::MaterialNotFileError);
            return;
        }
        // not ends with MTL|mtl extension
        if ((!path.has_extension()) ||
            ((path.extension() != ".mtl") &&
            (path.extension() != ".MTL")))
        {
            err.code = make_error_code(yyobj_errc::MaterialInvalidExtError);
            return;
        }
        /* Get MTL file's directory */
        auto curr_dir = path.parent_path();
        FILE* file{};
        errno_t cerr{};
        cerr = fopen_s(&file, path.string().c_str(), "rb");
        /* Open MTL file error */
        if (cerr)
        {
            err.code = make_error_code(yyobj_errc::MaterialOpenFileError);
            return;
        }
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        /* Allocate mtllib buffer */
        char* buffer = new char[size + 1];
        /* Can't alloc buffer for mtl file bytes */
        if (!buffer)
        {
            err.code = make_error_code(yyobj_errc::MaterialAllocBufferError);
            fclose(file);
            return;
        }
        // In most case, mtl file is much smaller than obj file
        // So read it just one time
        size_t length = fread(buffer, 1, size, file);
        // read size and file size don't match
        if (size != length)
        {
            err.code = make_error_code(yyobj_errc::MaterialFileError);
            fclose(file);
            return;
        }
        buffer[length] = '\n';
        /* file cleanup function */
        auto cleanup = [&buffer, &file]() {
            delete[]buffer;
            fclose(file);
        };
        yyobj::Material tempMtl{};
        /* parse start */
        // initialize mtl file context
        ctx.line = 1;
        err.message = path.string();

        const char* p = buffer;
        const char* e = buffer + length;
        bool found_d = false;
        const char* line_start;
        while (p < e)
        {
            line_start = p;
            skip_whitespace(p);
            switch (*p)
            {
            case 'n':
                p++;
                // newmtl ...
                if (p[0] == 'e' &&
                    p[1] == 'w' &&
                    p[2] == 'm' &&
                    p[3] == 't' &&
                    p[4] == 'l' &&
                    is_whitespace(p[5]))
                {
                    if (tempMtl.name != "")
                    {
                        ctx.mesh->materials.push_back(std::move(tempMtl));
                        tempMtl = yyobj::Material{};
                        found_d = false;
                    }
                    /* Read name */
                    p += 5;
                    skip_whitespace(p);
                    /* empty name*/
                    if (*p == '\n')
                    {
                        goto parse_fail;
                    }
                    auto st = p;
                    skip_name(p);
                    tempMtl.name = std::string(st, p);
                }
                else
                {
                    goto parse_fail;
                }
                break;
            case 'K':
                // Ka
                if (p[1] == 'a')
                {
                    p += 2;
                    parse_mtl_tri_number(tempMtl.Ka, p, err);
                }
                // Kd
                else if (p[1] == 'd')
                {
                    p += 2;
                    parse_mtl_tri_number(tempMtl.Kd, p, err);
                }
                // Ks
                else if (p[1] == 's')
                {
                    p += 2;
                    parse_mtl_tri_number(tempMtl.Ks, p, err);
                }
                // Ke
                else if (p[1] == 'e')
                {
                    p += 2;
                    parse_mtl_tri_number(tempMtl.Ke, p, err);
                }
                // Kt
                else if (p[1] == 't')
                {
                    p += 2;
                    parse_mtl_tri_number(tempMtl.Kt, p, err);
                }
                else
                    goto parse_fail;
                break;
            case 'N':
                if (p[1] == 's')
                {
                    p += 2;
                    parse_mtl_one_number(tempMtl.Ns, p, err);
                }
                else if (p[1] == 'i')
                {
                    p += 2;
                    parse_mtl_one_number(tempMtl.Ni, p, err);
                }
                else
                {
                    goto parse_fail;
                }
                break;
            case 'T':
                // Tr
                if (p[1] == 'r')
                {
                    float Tr;
                    p += 2;
                    if (!found_d)
                    {
                        parse_mtl_one_number(Tr, p, err);
                        tempMtl.d = 1.0f - Tr;
                    }
                }
                // Tf
                else if (p[1] == 'f')
                {
                    p += 2;
                    parse_mtl_tri_number(tempMtl.Tf, p, err);
                }
                else
                {
                    goto parse_fail;
                }
                break;
            case 'd':
                // d
                p++;
                parse_mtl_one_number(tempMtl.d, p, err);
                found_d = true;
                break;
            case 'i':
                p++;
                // illum
                if (p[0] == 'l' &&
                    p[1] == 'l' &&
                    p[2] == 'u' &&
                    p[3] == 'm' &&
                    is_whitespace(p[4]))
                {
                    p += 4;
                    parse_mtl_one_number(tempMtl.illum, p, err);
                }
                else
                {
                    goto parse_fail;
                }
                break;
            case 'm':
                p++;
                if (p[0] == 'a' &&
                    p[1] == 'p' &&
                    p[2] == '_')
                {
                    p += 3;
                    if (*p == 'K')
                    {
                        p++;
                        // map_Ka
                        if (*p == 'a')
                        {
                            p++;
                            parse_mtl_map(ctx, tempMtl.map_Ka, p, curr_dir, err);
                        }
                        // map_Kd
                        else if (*p == 'd')
                        {
                            p++;
                            parse_mtl_map(ctx, tempMtl.map_Kd, p, curr_dir, err);
                        }
                        // map_Ks
                        else if (*p == 's')
                        {
                            p++;
                            parse_mtl_map(ctx, tempMtl.map_Ks, p, curr_dir, err);
                        }
                        // map_Ke
                        else if (*p == 'e')
                        {
                            p++;
                            parse_mtl_map(ctx, tempMtl.map_Ke, p, curr_dir, err);
                        }
                        // map_Kt
                        else if (*p == 't')
                        {
                            p++;
                            parse_mtl_map(ctx, tempMtl.map_Kt, p, curr_dir, err);
                        }
                        else
                        {
                            goto parse_fail;
                        }
                    }
                    else if (*p == 'N')
                    {
                        p++;
                        if (is_whitespace(p[1]))
                        {
                            // map_Ns
                            if (*p == 's')
                            {
                                parse_mtl_map(ctx, tempMtl.map_Ns, p, curr_dir, err);
                            }
                            // map_Ni
                            else if (*p == 'i')
                            {
                                parse_mtl_map(ctx, tempMtl.map_Ni, p, curr_dir, err);
                            }
                            else
                            {
                                goto parse_fail;
                            }
                        }
                        else
                        {
                            goto parse_fail;
                        }
                    }
                    // map_d
                    else if (*p == 'd')
                    {
                        p++;
                        if (is_whitespace(*p))
                        {
                            parse_mtl_map(ctx, tempMtl.map_d, p, curr_dir, err);
                        }
                        else
                        {
                            goto parse_fail;
                        }
                    }
                    // map_bump
                    else if ((p[0] == 'b' || p[0] == 'B') &&
                        p[1] == 'u' &&
                        p[2] == 'm' &&
                        p[3] == 'p' &&
                        is_whitespace(p[4]))
                    {
                        p += 4;
                        parse_mtl_map(ctx, tempMtl.map_bump, p, curr_dir, err);
                    }
                    else
                    {
                        goto parse_fail;
                    }
                }
                else
                {
                    goto parse_fail;
                }
                break;
            case '#':
                break;
            }
            if (err)
            {
                auto line_end = line_start;
                skip_line(line_end);
                err.line = std::string(line_start, line_end);
                err.line_num = ctx.line;
                cleanup();
                return;
            }
            skip_line(p);
            ctx.line++;
        }
        /* push final material */
        if (tempMtl.name != "")
        {
            ctx.mesh->materials.push_back(std::move(tempMtl));
        }
        cleanup();
        return;
    parse_fail:
        err.code = make_error_code(yyobj_errc::MaterialParseError);
        auto line_end = line_start;
        skip_line(line_end);
        err.line = std::string(line_start, line_end);
        err.line_num = ctx.line;
        cleanup();
        return;
    }
    template<typename T>
    static constexpr yyobj_errc int_or_float_error() {
        if constexpr (std::is_integral_v<T>) {
            return yyobj_errc::ParseIntError;
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return yyobj_errc::ParseFloatError;
        }
        else {
            return yyobj_errc::MaterialParseError;
        }
    }
    template<typename T>
    void parse_mtl_tri_number(T* arr, const char*& p, Error& err)
    {
        T v{};
        for (size_t i = 0; i < 3; i++)
        {
            skip_whitespace(p);
            auto result = fast_float::from_chars(p, char_nullptr, v);
            if (result.ec != std::errc())
            {
                err.code = make_error_code(int_or_float_error<T>());
                return;
            }
            p = result.ptr;
            arr[i] = v;
        }
        /* if there are still something after three float */
        /* FIXME: do we really need it? */
        skip_whitespace(p);
        if (*p != '\n')
        {
            err.code = make_error_code(int_or_float_error<T>());
        }
        return;
    }
    template<typename T>
    void parse_mtl_one_number(T& num, const char*& p, Error& err)
    {
        skip_whitespace(p);
        auto result = fast_float::from_chars(p, char_nullptr, num);
        if (result.ec != std::errc()) [[unlikely]]
        {
            err.code = make_error_code(int_or_float_error<T>());
            return;
        }
        p = result.ptr;
        /* if there are still something after three float */
        /* FIXME: do we really need it? */
        skip_whitespace(p);
        if (*p != '\n')
        {
            err.code = make_error_code(int_or_float_error<T>());
        }
        return;
    }
    void parse_mtl_map(Context& ctx, unsigned int& index, const char*& p, std::filesystem::path& mtlpath, Error& err)
    {
        skip_whitespace(p);
        if (*p == '-')
        {
            // fast_obj doesn't support options in map_* keywords, so do I.
            // It's rarely to see them in normal mtl files, so just ignoring
            err.code = make_error_code(yyobj_errc::MaterialNotSupportError);
            return;
        }
        if (*p == '\n')
        {
            err.code = make_error_code(yyobj_errc::MaterialParseError);
            return;
        }
        auto st = p;
        skip_name(p);
        std::string name(st, p);
        /* Start at ONE, skip dummy texture at ZERO place */
        index = 1;
        // FIXME: maybe we can use hashtable(unordered_map) here, but I dont know
        while (index < ctx.mesh->textures.size())
        {
            if (ctx.mesh->textures[index].name == name)
                break;
            index++;
        }
        // texture not found, add a new one
        if (index == ctx.mesh->textures.size())
        {
            Texture tex{};
            tex.name = name;
            std::filesystem::path path;
            if (name[1] != ':')
            {
                path = mtlpath / name;
            }
            else
            {
                path = name;
            }
            std::error_code errc;
            path = std::filesystem::canonical(path, errc);
            // file not exists
            if (errc)
            {
                err.code = make_error_code(yyobj_errc::MaterialTextureNotFoundError);
                return;
            }
            // not a file
            if (!std::filesystem::is_regular_file(path))
            {
                err.code = make_error_code(yyobj_errc::MaterialTextureNotFoundError);
                return;
            }
            tex.path = path.string();
            ctx.mesh->textures.push_back(tex);
        }
        return;
    }
}

// from mesh to one huge vertex vector.
namespace yyobj
{
    void Obj2Data(Mesh* mesh, std::vector<Vertex>& vs, std::vector<Recoder>& recs)
    {
        recs.clear();
        vs.clear();
        //vs.resize(mesh->indices.size());
        auto& positions = mesh->positions;
        auto& texcoords = mesh->texcoords;
        auto& normals = mesh->normals;
        auto& indices = mesh->indices;
        size_t vs_index{};
        Recoder rec{};
        for (auto& gmtl : mesh->usemtls)
        {
            auto start = vs_index;
            auto end = gmtl.face_offset + gmtl.face_count;
            auto io = gmtl.index_offset;
            auto mtl_id = gmtl.index;
            for (size_t i = gmtl.face_offset; i < end; i++)
            {
                auto num_vertices = mesh->face_vertices[i];
                auto no_normal = false;
                float calc_normal[3]{};
                // obj data don't provide normal data
                // so we calculate it by hands
                if (indices[io].n == 0)
                {
                    no_normal = true;
                    float a[3] = {
                        positions[indices[io].p * 3] - positions[indices[io + 1].p * 3],
                        positions[indices[io].p * 3 + 1] - positions[indices[io + 1].p * 3 + 1],
                        positions[indices[io].p * 3 + 2] - positions[indices[io + 1].p * 3 + 2],
                    };
                    float b[3] = {
                        positions[indices[io].p * 3] - positions[indices[io + 2].p * 3],
                        positions[indices[io].p * 3 + 1] - positions[indices[io + 2].p * 3 + 1],
                        positions[indices[io].p * 3 + 2] - positions[indices[io + 2].p * 3 + 2],
                    };
                    float cross[3] = {
                        a[1] * b[2] - a[2] * b[1],
                        a[2] * b[0] - a[0] * b[2],
                        a[0] * b[1] - a[1] * b[0],
                    };
                    float length = std::sqrtf(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
                    // normalize normal vector
                    cross[0] /= length, cross[1] /= length, cross[2] /= length;
                    calc_normal[0] = cross[0], calc_normal[1] = cross[1], calc_normal[2] = cross[2];
                }
                if (num_vertices == 3) [[likely]]
                {
                    for (size_t j = 0; j < num_vertices; j++)
                    {
                        vs.push_back({});
                        std::memcpy(vs[vs_index].pos, positions.data() + 3 * indices[io + j].p,
                            sizeof(float) * 3);
                        std::memcpy(vs[vs_index].tex, texcoords.data() + 2 * indices[io + j].t,
                            sizeof(float) * 2);
                        if (!no_normal)
                        {
                            std::memcpy(vs[vs_index].nor, normals.data() + 3 * indices[io + j].n,
                                sizeof(float) * 3);
                        }
                        else
                        {
                            // use calculated normal
                            std::memcpy(vs[vs_index].nor, calc_normal, sizeof(float) * 3);
                        }
                        vs_index++;
                    }
                }
                else if (num_vertices == 4)
                {
                    const auto& idx0 = indices[io + 0];
                    const auto& idx1 = indices[io + 1];
                    const auto& idx2 = indices[io + 2];
                    const auto& idx3 = indices[io + 3];
                    bool d02_is_less{};
                    {
                        size_t pos_idx0 = (size_t)3 * idx0.p;
                        size_t pos_idx1 = (size_t)3 * idx1.p;
                        size_t pos_idx2 = (size_t)3 * idx2.p;
                        size_t pos_idx3 = (size_t)3 * idx3.p;

                        auto pos0_x = positions[pos_idx0 + 0];
                        auto pos0_y = positions[pos_idx0 + 1];
                        auto pos0_z = positions[pos_idx0 + 2];

                        auto pos1_x = positions[pos_idx1 + 0];
                        auto pos1_y = positions[pos_idx1 + 1];
                        auto pos1_z = positions[pos_idx1 + 2];

                        auto pos2_x = positions[pos_idx2 + 0];
                        auto pos2_y = positions[pos_idx2 + 1];
                        auto pos2_z = positions[pos_idx2 + 2];

                        auto pos3_x = positions[pos_idx3 + 0];
                        auto pos3_y = positions[pos_idx3 + 1];
                        auto pos3_z = positions[pos_idx3 + 2];

                        auto e02_x = pos0_x - pos2_x;
                        auto e02_y = pos0_y - pos2_y;
                        auto e02_z = pos0_z - pos2_z;

                        auto e13_x = pos1_x - pos3_x;
                        auto e13_y = pos1_y - pos3_y;
                        auto e13_z = pos1_z - pos3_z;

                        auto d02 = e02_x * e02_x + e02_y * e02_y + e02_z * e02_z;
                        auto d13 = e13_x * e13_x + e13_y * e13_y + e13_z * e13_z;
                        d02_is_less = d02 < d13;
                    }
                    std::vector<VertexIndex> inds = {
                        idx0, idx1, d02_is_less ? idx2 : idx3,
                        d02_is_less ? idx0 : idx1, idx2, idx3
                    };
                    for (size_t j = 0; j < inds.size(); j++)
                    {
                        vs.push_back({});
                        std::memcpy(vs[vs_index].pos, positions.data() + 3 * inds[j].p,
                            sizeof(float) * 3);
                        std::memcpy(vs[vs_index].tex, texcoords.data() + 2 * inds[j].t,
                            sizeof(float) * 2);
                        if (!no_normal)
                        {
                            std::memcpy(vs[vs_index].nor, normals.data() + 3 * inds[j].n,
                                sizeof(float) * 3);
                        }
                        else
                        {
                            std::memcpy(vs[vs_index].nor, calc_normal, sizeof(float) * 3);
                        }
                        vs_index++;
                    }
                }
                else // 5, 6, 7, ... more vertex
                {
                    // phony implementation, just assert polygon are Convex
                    std::vector<VertexIndex> inds{};
                    for (int k = 1; k < num_vertices - 1; k++)
                    {
                        inds.push_back(indices[io]);
                        inds.push_back(indices[io + k]);
                        inds.push_back(indices[io + k + 1]);
                    }
                    for (size_t j = 0; j < inds.size(); j++)
                    {
                        vs.push_back({});
                        std::memcpy(vs[vs_index].pos, positions.data() + 3 * inds[j].p,
                            sizeof(float) * 3);
                        std::memcpy(vs[vs_index].tex, texcoords.data() + 2 * inds[j].t,
                            sizeof(float) * 2);
                        if (!no_normal)
                        {
                            std::memcpy(vs[vs_index].nor, normals.data() + 3 * inds[j].n,
                                sizeof(float) * 3);
                        }
                        else
                        {
                            std::memcpy(vs[vs_index].nor, calc_normal, sizeof(float) * 3);
                        }
                        vs_index++;
                    }
                }
                io += num_vertices;
            }
            rec.mtl_index = mtl_id;
            rec.offset = start;
            rec.count = vs_index - start;
            recs.push_back(rec);
        }
    }
}
