#pragma once
#ifdef	GLADDLL_EXPORTS
#define GLAD_GLAPI_EXPORT
#define WIN32
#define GLAD_GLAPI_EXPORT_BUILD
#else
#define GLAD_GLAPI_EXPORT
#define WIN32
#endif