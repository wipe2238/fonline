#ifndef __SPRITE_MANAGER__
#define __SPRITE_MANAGER__

#include "Common.h"
#include "Sprites.h"
#include "FileManager.h"
#include "Text.h"
#include "3dStuff.h"
#include "GraphicLoader.h"

// #define RENDER_3D_TO_2D

// Font flags
#define FT_NOBREAK                 ( 0x0001 )
#define FT_NOBREAK_LINE            ( 0x0002 )
#define FT_CENTERX                 ( 0x0004 )
#define FT_CENTERY                 ( 0x0008 )
#define FT_CENTERR                 ( 0x0010 )
#define FT_BOTTOM                  ( 0x0020 )
#define FT_UPPER                   ( 0x0040 )
#define FT_NO_COLORIZE             ( 0x0080 )
#define FT_ALIGN                   ( 0x0100 )
#define FT_BORDERED                ( 0x0200 )
#define FT_SKIPLINES( l )             ( 0x0400 | ( ( l ) << 16 ) )
#define FT_SKIPLINES_END( l )         ( 0x0800 | ( ( l ) << 16 ) )

// Colors
#define COLOR_RGBA( a, r, g, b )      ( (uint) ( ( ( ( a ) & 0xFF ) << 24 ) | ( ( ( r ) & 0xFF ) << 16 ) | ( ( ( g ) & 0xFF ) << 8 ) | ( ( b ) & 0xFF ) ) )
#define COLOR_RGB( r, g, b )          COLOR_RGBA( 0xFF, r, g, b )
#define COLOR_SWAP_RB( c )            ( ( ( c ) & 0xFF00FF00 ) | ( ( ( c ) & 0x00FF0000 ) >> 16 ) | ( ( ( c ) & 0x000000FF ) << 16 ) )
#define COLOR_CHANGE_ALPHA( v, a )    ( ( ( ( v ) | 0xFF000000 ) ^ 0xFF000000 ) | ( (uint) ( a ) & 0xFF ) << 24 )
#define COLOR_IFACE_FIX            COLOR_GAME_RGB( 103, 95, 86 )
#define COLOR_IFACE                SpriteManager::PackColor( ( ( COLOR_IFACE_FIX >> 16 ) & 0xFF ) + GameOpt.Light, ( ( COLOR_IFACE_FIX >> 8 ) & 0xFF ) + GameOpt.Light, ( COLOR_IFACE_FIX & 0xFF ) + GameOpt.Light )
#define COLOR_GAME_RGB( r, g, b )     SpriteManager::PackColor( ( r ) + GameOpt.Light, ( g ) + GameOpt.Light, ( b ) + GameOpt.Light )
#define COLOR_IFACE_RED            ( COLOR_IFACE | ( 0xFF << 16 ) )
#define COLOR_CRITTER_NAME         COLOR_GAME_RGB( 0xAD, 0xAD, 0xB9 )
#define COLOR_TEXT                 COLOR_GAME_RGB( 60, 248, 0 )
#define COLOR_TEXT_WHITE           COLOR_GAME_RGB( 0xFF, 0xFF, 0xFF )
#define COLOR_TEXT_DWHITE          COLOR_GAME_RGB( 0xBF, 0xBF, 0xBF )
#define COLOR_TEXT_RED             COLOR_GAME_RGB( 0xC8, 0, 0 )
#define COLOR_TEXT_DRED            COLOR_GAME_RGB( 0xAA, 0, 0 )
#define COLOR_TEXT_DDRED           COLOR_GAME_RGB( 0x66, 0, 0 )
#define COLOR_TEXT_DBLUE           COLOR_GAME_RGB( 0, 0, 0xAA )
#define COLOR_TEXT_GREEN           COLOR_GAME_RGB( 0, 0xC8, 0 )
#define COLOR_TEXT_DGREEN          COLOR_GAME_RGB( 0, 0xAA, 0 )
#define COLOR_TEXT_DDGREEN         COLOR_GAME_RGB( 0, 0x66, 0 )
#define COLOR_TEXT_LGREEN          COLOR_GAME_RGB( 0, 0xFF, 0 )
#define COLOR_TEXT_BLACK           COLOR_GAME_RGB( 0, 0, 0 )
#define COLOR_TEXT_DARK            COLOR_GAME_RGB( 0x30, 0x30, 0x30 )
#define COLOR_TEXT_SAND            COLOR_GAME_RGB( 0x8F, 0x6F, 0 )

// Sprite layers
#define DRAW_ORDER_FLAT            ( 0 )
#define DRAW_ORDER                 ( 20 )
#define DRAW_ORDER_TILE            ( DRAW_ORDER_FLAT + 0 )
#define DRAW_ORDER_TILE_END        ( DRAW_ORDER_FLAT + 4 )
#define DRAW_ORDER_HEX_GRID        ( DRAW_ORDER_FLAT + 5 )
#define DRAW_ORDER_FLAT_SCENERY    ( DRAW_ORDER_FLAT + 8 )
#define DRAW_ORDER_LIGHT           ( DRAW_ORDER_FLAT + 9 )
#define DRAW_ORDER_DEAD_CRITTER    ( DRAW_ORDER_FLAT + 10 )
#define DRAW_ORDER_FLAT_ITEM       ( DRAW_ORDER_FLAT + 13 )
#define DRAW_ORDER_TRACK           ( DRAW_ORDER_FLAT + 16 )
#define DRAW_ORDER_SCENERY         ( DRAW_ORDER + 3 )
#define DRAW_ORDER_ITEM            ( DRAW_ORDER + 6 )
#define DRAW_ORDER_CRITTER         ( DRAW_ORDER + 9 )
#define DRAW_ORDER_RAIN            ( DRAW_ORDER + 12 )
#define DRAW_ORDER_LAST            ( 39 )
#define DRAW_ORDER_ITEM_AUTO( i )     ( i->IsFlat() ? ( i->IsItem() ? DRAW_ORDER_FLAT_ITEM : DRAW_ORDER_FLAT_SCENERY ) : ( i->IsItem() ? DRAW_ORDER_ITEM : DRAW_ORDER_SCENERY ) )
#define DRAW_ORDER_CRIT_AUTO( c )     ( c->IsDead() && !c->IsRawParam( MODE_NO_FLATTEN ) ? DRAW_ORDER_DEAD_CRITTER : DRAW_ORDER_CRITTER )

// Sprites cutting
#define SPRITE_CUT_HORIZONTAL      ( 1 )
#define SPRITE_CUT_VERTICAL        ( 2 )
#define SPRITE_CUT_CUSTOM          ( 3 )               // Todo

// Egg types
#define EGG_ALWAYS                 ( 1 )
#define EGG_X                      ( 2 )
#define EGG_Y                      ( 3 )
#define EGG_X_AND_Y                ( 4 )
#define EGG_X_OR_Y                 ( 5 )

// Egg types
#define EGG_ALWAYS                 ( 1 )
#define EGG_X                      ( 2 )
#define EGG_Y                      ( 3 )
#define EGG_X_AND_Y                ( 4 )
#define EGG_X_OR_Y                 ( 5 )

// Contour types
#define CONTOUR_RED                ( 1 )
#define CONTOUR_YELLOW             ( 2 )
#define CONTOUR_CUSTOM             ( 3 )

// Primitives
#define PRIMITIVE_POINTLIST        ( 1 )
#define PRIMITIVE_LINELIST         ( 2 )
#define PRIMITIVE_LINESTRIP        ( 3 )
#define PRIMITIVE_TRIANGLELIST     ( 4 )
#define PRIMITIVE_TRIANGLESTRIP    ( 5 )
#define PRIMITIVE_TRIANGLEFAN      ( 6 )

class SpriteManager
{
private:
    Matrix          projectionMatrixCM;
    int             modeWidth, modeHeight;
    bool            sceneBeginned;
    RenderTarget*   rtMain;
    RenderTarget*   rtContours, * rtContoursMid;
    RenderTarget*   rt3DSprite, * rt3DMSSprite;
    #ifdef RENDER_3D_TO_2D
    RenderTarget*   rt3D, * rt3DMS;
    #endif
    RenderTargetVec rtStack;
    GLint           baseFBO;

public:
    static AnyFrames* DummyAnimation;

    SpriteManager();
    bool Init();
    void Finish();
    bool BeginScene( uint clear_color );
    void EndScene();

    // Render targets
    bool          IsRenderTargetSupported();
    RenderTarget* CreateRenderTarget( bool depth_stencil, bool multisampling = false, uint width = 0, uint height = 0, bool tex_linear = false );
    void          DeleteRenderTarget( RenderTarget*& rt );
    void          PushRenderTarget( RenderTarget* rt );
    void          PopRenderTarget();
    void          DrawRenderTarget( RenderTarget* rt, bool alpha_blend, const Rect* region_from = NULL, const Rect* region_to = NULL );
    void          ClearCurrentRenderTarget( uint color );
    void          ClearCurrentRenderTargetDS( bool depth, bool stencil );
    void          RefreshViewPort();

    // Texture atlases
public:
    void PushAtlasType( int atlas_type, bool one_image = false );
    void PopAtlasType();
    void AccumulateAtlasData();
    void FlushAccumulatedAtlasData();
    bool IsAccumulateAtlasActive();
    void FinalizeAtlas( int atlas_type );
    void AutofinalizeAtlases( int atlas_type );
    void DestroyAtlases( int atlas_type );
    void DumpAtlases();
    void SaveTexture( Texture* tex, const char* fname, bool flip );   // tex == NULL is back buffer

private:
    int             atlasWidth, atlasHeight;
    IntVec          atlasTypeStack;
    BoolVec         atlasOneImageStack;
    TextureAtlasVec allAtlases;
    bool            accumulatorActive;
    SprInfoVec      accumulatorSprInfo;
    IntVec          autofinalizeAtlases;
    PtrVec          atlasDataPool;

    TextureAtlas* CreateAtlas( int w, int h );
    TextureAtlas* FindAtlasPlace( SpriteInfo* si, int& x, int& y );
    uint          RequestFillAtlas( SpriteInfo* si, uint w, uint h, uchar* data );
    void          FillAtlas( SpriteInfo* si );

    // Load sprites
public:
    AnyFrames*   LoadAnimation( const char* fname, int path_type, bool use_dummy = false, bool frm_anim_pix = false );
    AnyFrames*   ReloadAnimation( AnyFrames* anim, const char* fname, int path_type );
    Animation3d* LoadPure3dAnimation( const char* fname, int path_type );
    void         FreePure3dAnimation( Animation3d* anim3d );
    bool         SaveAnimationInFastFormat( AnyFrames* anim, const char* fname, int path_type );
    bool         TryLoadAnimationInFastFormat( const char* fname, int path_type, FileManager& fm, AnyFrames*& anim );

private:
    SprInfoVec sprData;

    AnyFrames* CreateAnimation( uint frames, uint ticks );
    AnyFrames* LoadAnimationFrm( const char* fname, int path_type, bool anim_pix );
    AnyFrames* LoadAnimationRix( const char* fname, int path_type );
    AnyFrames* LoadAnimationFofrm( const char* fname, int path_type );
    AnyFrames* LoadAnimation3d( const char* fname, int path_type );
    AnyFrames* LoadAnimationArt( const char* fname, int path_type );
    AnyFrames* LoadAnimationSpr( const char* fname, int path_type );
    AnyFrames* LoadAnimationZar( const char* fname, int path_type );
    AnyFrames* LoadAnimationTil( const char* fname, int path_type );
    AnyFrames* LoadAnimationMos( const char* fname, int path_type );
    AnyFrames* LoadAnimationBam( const char* fname, int path_type );
    AnyFrames* LoadAnimationOther( const char* fname, int path_type, uchar * ( *loader )( const uchar *, uint, uint &, uint & ) );
    bool Render3d( int x, int y, float scale, Animation3d* anim3d, RectF* stencil, uint color );
    bool Render3dSize( RectF rect, bool stretch_up, bool center, Animation3d* anim3d, RectF* stencil, uint color );
    uint Render3dSprite( Animation3d* anim3d, int dir, int time_proc );

    // Draw
public:
    static uint PackColor( int r, int g, int b );
    void        SetSpritesColor( uint c ) { baseColor = c; }
    uint        GetSpritesColor()         { return baseColor; }
    SprInfoVec& GetSpritesInfo()          { return sprData; }
    SpriteInfo* GetSpriteInfo( uint id )  { return sprData[ id ]; }
    void        GetDrawRect( Sprite* prep, Rect& rect );
    uint        GetPixColor( uint spr_id, int offs_x, int offs_y, bool with_zoom = true );
    bool        IsPixNoTransp( uint spr_id, int offs_x, int offs_y, bool with_zoom = true );
    bool        IsEggTransp( int pix_x, int pix_y );

    void PrepareSquare( PointVec& points, Rect r, uint color );
    void PrepareSquare( PointVec& points, Point lt, Point rt, Point lb, Point rb, uint color );
    bool PrepareBuffer( Sprites& dtree, GLuint atlas, int ox, int oy, uchar alpha );
    bool Flush();

    bool DrawSprite( uint id, int x, int y, uint color = 0 );
    bool DrawSpritePattern( uint id, int x, int y, int w, int h, int spr_width = 0, int spr_height = 0, uint color = 0 );
    bool DrawSpriteSize( uint id, int x, int y, float w, float h, bool stretch_up, bool center, uint color = 0 );
    bool DrawSprites( Sprites& dtree, bool collect_contours, bool use_egg, int draw_oder_from, int draw_oder_to );
    bool DrawPoints( PointVec& points, int prim, float* zoom = NULL, RectF* stencil = NULL, PointF* offset = NULL, Effect* effect = NULL );
    bool Draw3d( int x, int y, float scale, Animation3d* anim3d, RectF* stencil, uint color );
    bool Draw3dSize( RectF rect, bool stretch_up, bool center, Animation3d* anim3d, RectF* stencil, uint color );

    inline bool DrawSprite( AnyFrames* frames, int x, int y, uint color = 0 )
    {
        if( frames && frames != DummyAnimation )
            return DrawSprite( frames->GetCurSprId(), x, y, color );
        return false;
    }
    inline bool DrawSpriteSize( AnyFrames* frames, int x, int y, float w, float h, bool stretch_up, bool center, uint color = 0 )
    {
        if( frames && frames != DummyAnimation )
            return DrawSpriteSize( frames->GetCurSprId(), x, y, w, h, stretch_up, center, color );
        return false;
    }

private:
    struct VertexArray
    {
        GLuint       VAO;
        GLuint       VBO;
        GLuint       IBO;
        uint         VCount;
        uint         ICount;
        VertexArray* Next;
    };
    VertexArray* quadsVertexArray;
    VertexArray* pointsVertexArray;
    UShortVec    quadsIndicies;
    UShortVec    pointsIndicies;
    Vertex2DVec  vBuffer;
    DipDataVec   dipQueue;
    uint         baseColor;
    int          drawQuadCount;
    int          curDrawQuad;

    void InitVertexArray( VertexArray* va, bool quads, uint count );
    void BindVertexArray( VertexArray* va );
    void EnableVertexArray( VertexArray* va, uint vertices_count );
    void DisableVertexArray( VertexArray*& va );
    void EnableStencil( RectF& stencil );
    void DisableStencil( bool clear_stencil );

    // Contours
public:
    bool DrawContours();

private:
    bool contoursAdded;

    bool CollectContour( int x, int y, SpriteInfo* si, Sprite* spr );

    // Transparent egg
private:
    bool        eggValid;
    ushort      eggHx, eggHy;
    int         eggX, eggY;
    SpriteInfo* sprEgg;
    int         eggSprWidth, eggSprHeight;
    float       eggAtlasWidth, eggAtlasHeight;

public:
    void InitializeEgg( const char* egg_name );
    bool CompareHexEgg( ushort hx, ushort hy, int egg_type );
    void SetEgg( ushort hx, ushort hy, Sprite* spr );
    void EggNotValid() { eggValid = false; }

    // Fonts
private:
    void BuildFont( int index );

public:
    void SetDefaultFont( int index, uint color );
    void SetFontEffect( int index, Effect* effect );
    bool LoadFontFO( int index, const char* font_name, bool not_bordered );
    bool LoadFontBMF( int index, const char* font_name );
    void BuildFonts();
    bool DrawStr( const Rect& r, const char* str, uint flags, uint color = 0, int num_font = -1 );
    int  GetLinesCount( int width, int height, const char* str, int num_font = -1 );
    int  GetLinesHeight( int width, int height, const char* str, int num_font = -1 );
    int  GetLineHeight( int num_font = -1 );
    void GetTextInfo( int width, int height, const char* str, int num_font, int flags, int& tw, int& th, int& lines );
    int  SplitLines( const Rect& r, const char* cstr, int num_font, StrVec& str_vec );
    bool HaveLetter( int num_font, const char* letter );
};

extern SpriteManager SprMngr;

#endif // __SPRITE_MANAGER__
