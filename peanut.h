#ifndef PEANUT_WORKAROUND_H_INCLUDED
#define PEANUT_WORKAROUND_H_INCLUDED

/* this file is for exposing peanut_gb.h stuff, without including that file twice. */

struct gb_s;
uint_fast32_t gb_get_save_size(struct gb_s *gb);

/* PEANUT_WORKAROUND_H_INCLUDED */
#endif
