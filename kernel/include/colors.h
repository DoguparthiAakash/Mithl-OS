#ifndef COLORS_H
#define COLORS_H

// Enhanced color system
#ifndef COLOR_BLACK
#define COLOR_BLACK 0x00
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE 0x01
#endif
// ... assuming cleanup, but honestly it's better to just not use conflicting names
// For now, let's just leave it as is, because I protected graphics.h instead.
// Actually, graphics.h included types.h, and kernel.c includes colors.h.
// The warnings were redefinition in graphics.h. I fixed graphics.h to check ifndef, so this file is fine as the "first" definer if included first.
// However, kernel.c includes colors.h BEFORE graphics.h. So COLOR_BLACK becomes 0x00.
// THIS IS BAD. We want 0xFF000000.
// So I should REMOVE colors.h from kernel.c or ensure graphics.h wins.


#endif // COLORS_H