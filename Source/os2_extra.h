// These are things that are in Warp 4 (some fixpack) but aren't in my
// copy of the toolkit.

#define WM_VRNDISABLED             0x007e
#define WM_VRNENABLED              0x007f

#define WM_MOUSEENTER              0x041E
#define WM_MOUSELEAVE              0x041F

extern "C" {

BOOL APIENTRY WinSetVisibleRegionNotify( HWND hwnd,
                                        BOOL fEnable);

ULONG APIENTRY WinQueryVisibleRegion( HWND hwnd,
                                     HRGN hrgn);


}
