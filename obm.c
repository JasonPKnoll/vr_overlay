//Make it so we don't need to include any other C files in our build.
#define CNFG_IMPLEMENTATION
#define  CNFGOGL
#include "rawdraw_sf.h"

#undef EXTERN_C
#include "openvr_capi.h"

intptr_t VR_InitInternal( EVRInitError *peError, EVRApplicationType eType );
void VR_ShutdownInternal();
bool VR_IsHmdPresent();
intptr_t VR_GetGenericInterface( const char *pchInterfaceVersion, EVRInitError *peError );
bool VR_IsRuntimeInstalled();
const char * VR_GetVRInitErrorAsSymbol( EVRInitError error );
const char * VR_GetVRInitErrorAsEnglishDescription( EVRInitError error );

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }

//global
struct VR_IVRSystem_FnTable * oSystem;
struct VR_IVROverlay_FnTable * oOverlay;
VROverlayHandle_t ulHandle;

void * CNOVRGetOpenVRFunctionTable( const char * interfacename )
{
	EVRInitError e;
	char fnTableName[128];
	int result1 = snprintf( fnTableName, 128, "FnTable:%s", interfacename );
	void * ret = (void *)VR_GetGenericInterface(fnTableName, &e );
	printf( "Getting System FnTable: %s = %p (%d)", fnTableName, ret, e );
	if( !ret )
	{
		exit( 1 );
	}
	return ret;
}

int AssociatedOverlay()
{
    // Return nonzero if fails, or 0 if success.
    TrackedDeviceIndex_t leftHandID;
    leftHandID = oSystem->GetTrackedDeviceIndexForControllerRole( ETrackedControllerRole_TrackedControllerRole_LeftHand );
    if( leftHandID == 0 || leftHandID == -1 ) return -9;

    struct HmdMatrix34_t transform = { 0 };
    transform.m[0][0] = 1;
    transform.m[1][1] = 1;
    transform.m[2][2] = 1;

    oOverlay->SetOverlayTransformTrackedDeviceRelative( ulHandle, leftHandID, &transform );
}


#define WIDTH 256
#define HEIGHT 256

int main()
{
    int has_associated_overlay;
	CNFGSetup( "Example App", WIDTH, HEIGHT );

    {
        EVRInitError e;
        VR_InitInternal( &e, EVRApplicationType_VRApplication_Overlay);
        if ( e != EVRInitError_VRInitError_None)
        {
            printf( "Error initializing OpenVR %d\n", e);
            return -5;
        }
    }

    printf( "OpenVR Started Succesfully" );

    oOverlay = (struct VR_IVROverlay_FnTable *)CNOVRGetOpenVRFunctionTable( IVROverlay_Version );
    oSystem = (struct VR_IVRSystem_FnTable *)CNOVRGetOpenVRFunctionTable( IVRSystem_Version );

    if( !oOverlay || !oSystem )
    {
        printf( "Error getting function tables from OpenVR\n" );
        return -9;
    }

    printf( "%p %p\n", oOverlay, oSystem );

    oOverlay->CreateOverlay( "obmtest-overlay", "obmtest", &ulHandle );
    oOverlay->SetOverlayWidthInMeters( ulHandle, 0.3 );
    oOverlay->SetOverlayColor( ulHandle, 1, 1, 1 );

    VRTextureBounds_t bounds;
    bounds.uMin = 0;
    bounds.uMax = 1;
    bounds.vMin = 0;
    bounds.vMax = 1;
    oOverlay->SetOverlayTextureBounds( ulHandle, &bounds );

    oOverlay->ShowOverlay( ulHandle );

    // Target texture to write into
    GLuint texture;
    glGenTextures( 1, &texture );
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  

	while(1)
	{
		CNFGBGColor = 0x000080ff; //Dark Blue Background
		CNFGClearFrame();
		CNFGHandleInput();

		//Change color to white.
		CNFGColor( 0xffffffff ); 

		CNFGPenX = 1; CNFGPenY = 1;
		CNFGDrawText( "Hello, World", 2 );

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();

        if( !has_associated_overlay )
        {
            if( AssociatedOverlay() == 0 )
            {
                has_associated_overlay = 1;
            }
        }

        // Shows the texture in VR
        glBindTexture( GL_TEXTURE_2D, texture );
        glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, WIDTH, HEIGHT, 0 );
        struct Texture_t OverlayTexture;
        OverlayTexture.eType = ETextureType_TextureType_OpenGL;
        OverlayTexture.eColorSpace = EColorSpace_ColorSpace_Auto;
        OverlayTexture.handle = texture;
        oOverlay->SetOverlayTexture( ulHandle, &OverlayTexture );
	}
}