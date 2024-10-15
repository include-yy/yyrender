#pragma once

namespace yyobj
{
    struct Vertex
    {
        float pos[3]{}; // Position
        float tex[2]{}; // TexCoord
        float nor[3]{}; // Normal
    };
    struct VertexIndex
    {
        unsigned int p{}; // Index of position
        unsigned int t{}; // Index of texcoord
        unsigned int n{}; // Index of normal
    };
    struct Texture
    {
        /* Texture name from .mtl file */
        std::string name;
        /* Resolved path to texture */
        std::string path;
    };
    // Material Data Structure
    // https://paulbourke.net/dataformats/mtl/
    // https://gist.github.com/hakanai/9baa46101488f5b857bc7a823a7231fd
    struct Material
    {
        // Inherited from fast_obj's fastObjMaterial Struct
        // https://github.com/thisistherk/fast_obj/blob/master/fast_obj.h#L51

        /* Material Name */
        std::string  name;

        /* Parameters */
        float        Ka[3]{};                   /* Ambient*/
        float        Kd[3]{ 1.0f, 1.0f, 1.0f }; /* Diffuse*/
        float        Ks[3]{};                   /* Specular */
        float        Ke[3]{};                   /* Emission */
        float        Kt[3]{};                   /* Transmittance */
        float        Ns{ 1.0f };                /* Shininess */
        float        Ni{ 1.0f };                /* Index of refraction */
        float        Tf[3]{ 1.0f, 1.0f, 1.0f }; /* Transmission filter */
        float        d{ 1.0f };                 /* Disolve (alpha) */
        int          illum{ 1 };                /* Illumination model */

        /* Set for materials that don't come from the associated mtllib */
        int          fallback{ 0 };

        /* Testure map indices in Mesh textures array */
        unsigned int map_Ka{};
        unsigned int map_Kd{};
        unsigned int map_Ks{};
        unsigned int map_Ke{};
        unsigned int map_Kt{};
        unsigned int map_Ns{};
        unsigned int map_Ni{};
        unsigned int map_d{};
        unsigned int map_bump{};
    };
    struct Group
    {
        /* Group or Object name */
        std::string name;
        /* Number of faces */
        unsigned int face_count{};
        /* First face in Mesh's face arrays */
        unsigned int face_offset{};
        /* First index in Mesh indices array */
        unsigned int index_offset{};
    };
    struct MtlGroup
    {
        /* Material index */
        unsigned int index{};
        /* Number of faces */
        unsigned int face_count{};
        /* First face in Mesh's face arrays */
        unsigned int face_offset{};
        /* First index in Mesh indices array */
        unsigned int index_offset{};
    };
    struct Mesh
    {
        /* Vertex data */
        unsigned int position_count{};
        std::vector<float> positions;
        unsigned int texcoord_count{};
        std::vector<float> texcoords;
        unsigned int normal_count{};
        std::vector<float> normals;
        unsigned int color_count{};
        std::vector<float> colors;

        /* Face data: one element for each face */
        std::vector<unsigned int> face_vertices;
        std::vector<unsigned int> face_materials;

        /* Index data: one element for each face vertex */
        std::vector<VertexIndex> indices;

        /* Materials */
        std::vector<Material> materials;

        /* Texture maps */
        std::vector<Texture> textures;

        /* Mesh objects ('o' tag in .obj file) */
        std::vector<Group> objects;

        /* Mesh groups ('g' tag in .obj file) */
        std::vector<Group> groups;

        /* Material groups ('usemtl' tag in .obj file */
        std::vector<MtlGroup> usemtls;
    };
    struct Error final
    {
        explicit operator bool() const noexcept;
        std::error_code code{};
        std::string message{};
        /* File parse related info */
        std::string line{};
        size_t line_num{};
    };

    enum class yyobj_errc
    {
        Success,
        /* Error related to obj file */
        ObjInvalidPathError,
        ObjNotFileError,
        ObjInvalidExtError,
        ObjOpenFileError,
        ObjAllocBufferError,
        ObjInvalidKeywordError,
        MaterialNotFoundError,
        ParseFloatError,
        ParseIntError,
        ParseIndexError,
        ParseArityMismatchError,
        /* Error related to mtl file */
        MaterialFileError,
        MaterialInvalidPathError,
        MaterialNotFileError,
        MaterialInvalidExtError,
        MaterialOpenFileError,
        MaterialAllocBufferError,
        MaterialParseError,
        MaterialNotSupportError,
        MaterialTextureNotFoundError,
        /* Error related to conversion data */
        TriangulationError,
        /* fallback error */
        InternalError
    };

    struct [[nodiscard]] Result final
    {
        std::unique_ptr<Mesh> mesh;
        Error error;
    };
    struct Recoder final
    {
        unsigned int offset;
        unsigned int count;
        unsigned int mtl_index;
    };
    Result ParseObjFile(std::string path);
    void Obj2Data(Mesh* mesh, std::vector<Vertex>& vs, std::vector<Recoder>& recs);

#ifndef YYOBJ_EXPORT
    struct yyobj_error_category : std::error_category
    {
        const char* name() const noexcept override;
        std::string message(int err) const override;
    };
    std::error_code make_error_code(yyobj_errc err);

    /* Context used in parsing OBJ */
    struct Context
    {
        std::unique_ptr<Mesh> mesh{};

        /* Current object/group */
        Group object{};
        Group group{};

        /* Current material group */
        MtlGroup mtl{};

        /* Current line in file */
        unsigned int line{ 1 };

        /* OBJ's parent directory */
        std::filesystem::path dir;
    };
    static const size_t BUFFER_SIZE = 65536; // 64KB
    //static const size_t BUFFER_SIZE = 262144; // 256KB
    static const size_t BUFFER_ALIGN = 4096; // 4k
    static const char* char_nullptr = nullptr;
    /* position/texcoord/normal/face */
    void parse_buffer(Context& ctx, const char* ptr, const char* end, Error& err);
    void parse_vertex(Context& ctx, const char*& ptr, Error& err);
    void parse_texcoord(Context& ctx, const char*& ptr, Error& err);
    void parse_normal(Context& ctx, const char*& ptr, Error& err);
    void parse_face(Context& ctx, const char*& ptr, Error& err);
    /* object/group/usemtl */
    void flush_object(Context& ctx);
    void parse_object(Context& ctx, const char*& ptr, Error& err);
    void flush_group(Context& ctx);
    void parse_group(Context& ctx, const char*& ptr, Error& err);
    void flush_usemtl(Context& ctx);
    void parse_usemtl(Context& ctx, const char*& ptr, Error& err);
    /* material parsing */
    void parse_mtllib(Context& ctx, const char*& ptr, Error& err);
    template<typename T>
    void parse_mtl_tri_number(T* arr, const char*& ptr, Error& err);
    template<typename T>
    void parse_mtl_one_number(T& num, const char*& ptr, Error& err);
    void parse_mtl_map(Context& ctx, unsigned int& index, const char*& ptr, std::filesystem::path& mtlpath, Error& err);
#endif
}
