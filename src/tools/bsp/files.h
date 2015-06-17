//
//  files.h
//  bsp
//
//  Created by Bryce Kelleher on 27/11/2014.
//  Copyright (c) 2014 Bryce Kelleher. All rights reserved.
//

#ifndef bsp_files_h
#define bsp_files_h

//definition of files

// file sections
typedef struct header_s
{
    int	sectionlistoffset;
} header_t;

// entry for the section table
typedef struct section_s
{
    char	name[16];
    int	offset;
    int	size;
} section_t;



// bsp disk structures

#endif
