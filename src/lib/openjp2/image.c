/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opj_includes.h"

opj_image_t* opj_image_create0(void)
{
    opj_image_t *image = (opj_image_t*)opj_calloc(1, sizeof(opj_image_t));
    return image;
}

opj_image_t* OPJ_CALLCONV opj_image_create(OPJ_UINT32 numcmpts,
        opj_image_cmptparm_t *cmptparms, OPJ_COLOR_SPACE clrspc)
{
    OPJ_UINT32 compno;
    opj_image_t *image = NULL;

    image = (opj_image_t*) opj_calloc(1, sizeof(opj_image_t));
    if (image) {
        image->color_space = clrspc;
        image->numcomps = numcmpts;
        /* allocate memory for the per-component information */
        image->comps = (opj_image_comp_t*)opj_calloc(image->numcomps,
                       sizeof(opj_image_comp_t));
        if (!image->comps) {
            /* TODO replace with event manager, breaks API */
            /* fprintf(stderr,"Unable to allocate memory for image.\n"); */
            opj_image_destroy(image);
            return NULL;
        }
        /* create the individual image components */
        for (compno = 0; compno < numcmpts; compno++) {
            opj_image_comp_t *comp = &image->comps[compno];
            comp->dx = cmptparms[compno].dx;
            comp->dy = cmptparms[compno].dy;
            comp->w = cmptparms[compno].w;
            comp->h = cmptparms[compno].h;
            comp->x0 = cmptparms[compno].x0;
            comp->y0 = cmptparms[compno].y0;
            comp->prec = cmptparms[compno].prec;
            comp->sgnd = cmptparms[compno].sgnd;
            if (comp->h != 0 &&
                    (OPJ_SIZE_T)comp->w > SIZE_MAX / comp->h / sizeof(OPJ_INT32)) {
                /* TODO event manager */
                opj_image_destroy(image);
                return NULL;
            }
            comp->data = (OPJ_INT32*) opj_image_data_alloc(
                             (size_t)comp->w * comp->h * sizeof(OPJ_INT32));
            if (!comp->data) {
                /* TODO replace with event manager, breaks API */
                /* fprintf(stderr,"Unable to allocate memory for image.\n"); */
                opj_image_destroy(image);
                return NULL;
            }
            memset(comp->data, 0, (size_t)comp->w * comp->h * sizeof(OPJ_INT32));
        }
    }

    return image;
}

void OPJ_CALLCONV opj_image_destroy(opj_image_t *image)
{
    if (image) {
        OPJ_UINT32 i;

        if (image->comps) {
            OPJ_UINT32 compno;

            /* image components */
            for (compno = 0; compno < image->numcomps; compno++) {
                opj_image_comp_t *image_comp = &(image->comps[compno]);
                if (image_comp->data) {
                    opj_image_data_free(image_comp->data);
                }
            }
            opj_free(image->comps);
        }

        if (image->icc_profile_buf) {
            opj_free(image->icc_profile_buf);
        }

        /* XML (XMP) metadata boxes */
        for (i = 0; i < image->num_xml && i < OPJ_JP2_MAX_XML_BOXES; ++i) {
            if (image->xml[i]) {
                opj_free(image->xml[i]);
                image->xml[i] = NULL;
            }
        }
        image->num_xml = 0;

        /* UUID metadata boxes */
        if (image->uuids) {
            for (i = 0; i < image->num_uuids; ++i) {
                if (image->uuids[i].data) {
                    opj_free(image->uuids[i].data);
                }
            }
            opj_free(image->uuids);
            image->uuids = NULL;
            image->num_uuids = 0;
        }

        opj_free(image);
    }
}

OPJ_BOOL OPJ_CALLCONV opj_image_add_xml(opj_image_t *image,
                                        const OPJ_BYTE *data,
                                        OPJ_UINT32 length)
{
    OPJ_BYTE *l_buf;

    if (!image || (!data && length != 0)) {
        return OPJ_FALSE;
    }
    if (image->num_xml >= OPJ_JP2_MAX_XML_BOXES) {
        return OPJ_FALSE;
    }

    l_buf = (OPJ_BYTE*)opj_malloc(length ? length : 1);
    if (!l_buf) {
        return OPJ_FALSE;
    }
    if (length) {
        memcpy(l_buf, data, length);
    }

    image->xml[image->num_xml] = l_buf;
    image->xml_len[image->num_xml] = length;
    image->num_xml++;

    return OPJ_TRUE;
}

OPJ_UINT32 OPJ_CALLCONV opj_image_get_num_xml_boxes(const opj_image_t *image)
{
    if (!image) {
        return 0;
    }
    return image->num_xml;
}

const OPJ_BYTE * OPJ_CALLCONV opj_image_get_xml_box(const opj_image_t *image,
        OPJ_UINT32 idx,
        OPJ_UINT32 *p_length)
{
    if (!image || idx >= image->num_xml || idx >= OPJ_JP2_MAX_XML_BOXES) {
        return NULL;
    }
    if (p_length) {
        *p_length = image->xml_len[idx];
    }
    return image->xml[idx];
}

OPJ_BOOL OPJ_CALLCONV opj_image_add_uuid_box(opj_image_t *image,
        const OPJ_BYTE uuid[16],
        const OPJ_BYTE *data,
        OPJ_UINT32 length)
{
    opj_uuid_box_t *l_new_uuids;
    opj_uuid_box_t *l_box;

    if (!image || !uuid || (!data && length != 0)) {
        return OPJ_FALSE;
    }
    if (image->num_uuids == 0xFFFFFFFFU) {
        return OPJ_FALSE;
    }

    l_new_uuids = (opj_uuid_box_t*)opj_realloc(image->uuids,
                  (image->num_uuids + 1) * sizeof(opj_uuid_box_t));
    if (!l_new_uuids) {
        return OPJ_FALSE;
    }
    image->uuids = l_new_uuids;

    l_box = &image->uuids[image->num_uuids];
    memcpy(l_box->uuid, uuid, 16);
    l_box->data = NULL;
    l_box->data_len = 0;

    if (length) {
        l_box->data = (OPJ_BYTE*)opj_malloc(length);
        if (!l_box->data) {
            return OPJ_FALSE;
        }
        memcpy(l_box->data, data, length);
        l_box->data_len = length;
    }

    image->num_uuids++;

    return OPJ_TRUE;
}

OPJ_UINT32 OPJ_CALLCONV opj_image_get_num_uuid_boxes(const opj_image_t *image)
{
    if (!image) {
        return 0;
    }
    return image->num_uuids;
}

const opj_uuid_box_t * OPJ_CALLCONV opj_image_get_uuid_box(
    const opj_image_t *image,
    OPJ_UINT32 idx)
{
    if (!image || idx >= image->num_uuids) {
        return NULL;
    }
    return &image->uuids[idx];
}

/**
 * Updates the components characteristics of the image from the coding parameters.
 *
 * @param p_image_header    the image header to update.
 * @param p_cp              the coding parameters from which to update the image.
 */
void opj_image_comp_header_update(opj_image_t * p_image_header,
                                  const struct opj_cp * p_cp)
{
    OPJ_UINT32 i, l_width, l_height;
    OPJ_UINT32 l_x0, l_y0, l_x1, l_y1;
    OPJ_UINT32 l_comp_x0, l_comp_y0, l_comp_x1, l_comp_y1;
    opj_image_comp_t* l_img_comp = NULL;

    l_x0 = opj_uint_max(p_cp->tx0, p_image_header->x0);
    l_y0 = opj_uint_max(p_cp->ty0, p_image_header->y0);
    l_x1 = p_cp->tx0 + (p_cp->tw - 1U) *
           p_cp->tdx; /* validity of p_cp members used here checked in opj_j2k_read_siz. Can't overflow. */
    l_y1 = p_cp->ty0 + (p_cp->th - 1U) * p_cp->tdy; /* can't overflow */
    l_x1 = opj_uint_min(opj_uint_adds(l_x1, p_cp->tdx),
                        p_image_header->x1); /* use add saturated to prevent overflow */
    l_y1 = opj_uint_min(opj_uint_adds(l_y1, p_cp->tdy),
                        p_image_header->y1); /* use add saturated to prevent overflow */

    l_img_comp = p_image_header->comps;
    for (i = 0; i < p_image_header->numcomps; ++i) {
        l_comp_x0 = opj_uint_ceildiv(l_x0, l_img_comp->dx);
        l_comp_y0 = opj_uint_ceildiv(l_y0, l_img_comp->dy);
        l_comp_x1 = opj_uint_ceildiv(l_x1, l_img_comp->dx);
        l_comp_y1 = opj_uint_ceildiv(l_y1, l_img_comp->dy);
        l_width   = opj_uint_ceildivpow2(l_comp_x1 - l_comp_x0, l_img_comp->factor);
        l_height  = opj_uint_ceildivpow2(l_comp_y1 - l_comp_y0, l_img_comp->factor);
        l_img_comp->w = l_width;
        l_img_comp->h = l_height;
        l_img_comp->x0 = l_comp_x0;
        l_img_comp->y0 = l_comp_y0;
        ++l_img_comp;
    }
}


/**
 * Copy only header of image and its component header (no data are copied)
 * if dest image have data, they will be freed
 *
 * @param   p_image_src     the src image
 * @param   p_image_dest    the dest image
 *
 */
void opj_copy_image_header(const opj_image_t* p_image_src,
                           opj_image_t* p_image_dest)
{
    OPJ_UINT32 compno;

    /* preconditions */
    assert(p_image_src != 00);
    assert(p_image_dest != 00);

    p_image_dest->x0 = p_image_src->x0;
    p_image_dest->y0 = p_image_src->y0;
    p_image_dest->x1 = p_image_src->x1;
    p_image_dest->y1 = p_image_src->y1;

    if (p_image_dest->comps) {
        for (compno = 0; compno < p_image_dest->numcomps; compno++) {
            opj_image_comp_t *image_comp = &(p_image_dest->comps[compno]);
            if (image_comp->data) {
                opj_image_data_free(image_comp->data);
            }
        }
        opj_free(p_image_dest->comps);
        p_image_dest->comps = NULL;
    }

    p_image_dest->numcomps = p_image_src->numcomps;

    p_image_dest->comps = (opj_image_comp_t*) opj_malloc(p_image_dest->numcomps *
                          sizeof(opj_image_comp_t));
    if (!p_image_dest->comps) {
        p_image_dest->comps = NULL;
        p_image_dest->numcomps = 0;
        return;
    }

    for (compno = 0; compno < p_image_dest->numcomps; compno++) {
        memcpy(&(p_image_dest->comps[compno]),
               &(p_image_src->comps[compno]),
               sizeof(opj_image_comp_t));
        p_image_dest->comps[compno].data = NULL;
    }

    p_image_dest->color_space = p_image_src->color_space;
    p_image_dest->icc_profile_len = p_image_src->icc_profile_len;

    if (p_image_dest->icc_profile_len) {
        p_image_dest->icc_profile_buf = (OPJ_BYTE*)opj_malloc(
                                            p_image_dest->icc_profile_len);
        if (!p_image_dest->icc_profile_buf) {
            p_image_dest->icc_profile_buf = NULL;
            p_image_dest->icc_profile_len = 0;
            return;
        }
        memcpy(p_image_dest->icc_profile_buf,
               p_image_src->icc_profile_buf,
               p_image_src->icc_profile_len);
    } else {
        p_image_dest->icc_profile_buf = NULL;
    }

    return;
}

opj_image_t* OPJ_CALLCONV opj_image_tile_create(OPJ_UINT32 numcmpts,
        opj_image_cmptparm_t *cmptparms, OPJ_COLOR_SPACE clrspc)
{
    OPJ_UINT32 compno;
    opj_image_t *image = 00;

    image = (opj_image_t*) opj_calloc(1, sizeof(opj_image_t));
    if (image) {

        image->color_space = clrspc;
        image->numcomps = numcmpts;

        /* allocate memory for the per-component information */
        image->comps = (opj_image_comp_t*)opj_calloc(image->numcomps,
                       sizeof(opj_image_comp_t));
        if (!image->comps) {
            opj_image_destroy(image);
            return 00;
        }

        /* create the individual image components */
        for (compno = 0; compno < numcmpts; compno++) {
            opj_image_comp_t *comp = &image->comps[compno];
            comp->dx = cmptparms[compno].dx;
            comp->dy = cmptparms[compno].dy;
            comp->w = cmptparms[compno].w;
            comp->h = cmptparms[compno].h;
            comp->x0 = cmptparms[compno].x0;
            comp->y0 = cmptparms[compno].y0;
            comp->prec = cmptparms[compno].prec;
            comp->sgnd = cmptparms[compno].sgnd;
            comp->data = 0;
        }
    }

    return image;
}
