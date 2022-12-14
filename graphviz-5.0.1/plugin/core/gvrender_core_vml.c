/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <common/macros.h>
#include <common/const.h>
#include <common/types.h>
#include <common/utils.h>

#include <gvc/gvplugin_render.h>
#include <gvc/gvplugin_device.h>
#include <gvc/gvio.h>
#include <common/memory.h>
#include <cgraph/strcasecmp.h>
#include <cgraph/unreachable.h>

typedef enum { FORMAT_VML, FORMAT_VMLZ, } format_type;

unsigned int  graphHeight,graphWidth;

static void vml_bzptarray(GVJ_t * job, pointf * A, int n)
{
    int i;
    char *c;

    c = "m ";			/* first point */
    for (i = 0; i < n; i++) {
        /* integers only in path! */
	gvprintf(job, "%s%.0f,%.0f ", c, A[i].x, graphHeight-A[i].y);
	if (i == 0)
	    c = "c ";		/* second point */
	else
	    c = "";		/* remaining points */
    }
    gvputs(job, "\"");
}

static void vml_print_color(GVJ_t * job, gvcolor_t color)
{
    switch (color.type) {
    case COLOR_STRING:
	gvputs(job, color.u.string);
	break;
    case RGBA_BYTE:
	if (color.u.rgba[3] == 0) /* transparent */
	    gvputs(job, "none");
	else
	    gvprintf(job, "#%02x%02x%02x",
		color.u.rgba[0], color.u.rgba[1], color.u.rgba[2]);
	break;
    default:
	UNREACHABLE(); // internal error
    }
}

static void vml_grstroke(GVJ_t * job, int filled)
{
    (void)filled;

    obj_state_t *obj = job->obj;

    gvputs(job, "<v:stroke color=\"");
    vml_print_color(job, obj->pencolor);
    if (obj->penwidth != PENWIDTH_NORMAL)
	gvprintf(job, "\" weight=\"%.0fpt", obj->penwidth);
    if (obj->pen == PEN_DASHED) {
	gvputs(job, "\" dashstyle=\"dash");
    } else if (obj->pen == PEN_DOTTED) {
	gvputs(job, "\" dashstyle=\"dot");
    }
    gvputs(job, "\" />");
}


static void vml_grfill(GVJ_t * job, int filled)
{
    obj_state_t *obj = job->obj;

    if (filled){
        gvputs(job, " filled=\"true\" fillcolor=\"");
	vml_print_color(job, obj->fillcolor);
        gvputs(job, "\" ");
    }else{
	gvputs(job, " filled=\"false\" ");
    }
}

// wrapper around `xml_escape` to set flags for HTML escaping
static void html_puts(GVJ_t *job, const char *s) {
  const xml_flags_t flags = {.dash = 1, .nbsp = 1, .utf8 = 1};
  (void)xml_escape(s, flags, (int(*)(void*, const char*))gvputs, job);
}

static void vml_comment(GVJ_t * job, char *str)
{
    gvputs(job, "      <!-- ");
    html_puts(job, str);
    gvputs(job, " -->\n");
}
static void vml_begin_job(GVJ_t * job)
{
    gvputs(job, "<HTML>\n");
    gvputs(job, "\n<!-- Generated by ");
    html_puts(job, job->common->info[0]);
    gvputs(job, " version ");
    html_puts(job, job->common->info[1]);
    gvputs(job, " (");
    html_puts(job, job->common->info[2]);
    gvputs(job, ")\n-->\n");
}

static void vml_begin_graph(GVJ_t * job)
{
    obj_state_t *obj = job->obj;
    char *name;

    graphHeight = (unsigned)(job->bb.UR.y - job->bb.LL.y);
    graphWidth  = (unsigned)(job->bb.UR.x - job->bb.LL.x);

    gvputs(job, "<HEAD>");
    gvputs(job, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n");
 

    name = agnameof(obj->u.g);
    if (name[0]) {
        gvputs(job, "<TITLE>");
        html_puts(job, name);
        gvputs(job, "</TITLE>");
    }
    gvprintf(job, "<!-- Pages: %d -->\n", job->pagesArraySize.x * job->pagesArraySize.y);

/*  the next chunk and all the "DIV" stuff is not required,
 *  but it helps with non-IE browsers  */
    gvputs(job, "   <SCRIPT LANGUAGE='Javascript'>\n");
    gvputs(job, "   function browsercheck()\n");
    gvputs(job, "   {\n");
    gvputs(job, "      var ua = window.navigator.userAgent\n");
    gvputs(job, "      var msie = ua.indexOf ( 'MSIE ' )\n");
    gvputs(job, "      var ievers;\n");
    gvputs(job, "      var item;\n");
    gvputs(job, "      var VMLyes=new Array('_VML1_','_VML2_');\n");
    gvputs(job, "      var VMLno=new Array('_notVML1_','_notVML2_');\n");
    gvputs(job, "      if ( msie > 0 ){      // If Internet Explorer, return version number\n");
    gvputs(job, "         ievers= parseInt (ua.substring (msie+5, ua.indexOf ('.', msie )))\n");
    gvputs(job, "      }\n");
    gvputs(job, "      if (ievers>=5){\n");
    gvputs(job, "       for (x in VMLyes){\n");
    gvputs(job, "         item = document.getElementById(VMLyes[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='visible';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "       for (x in VMLno){\n");
    gvputs(job, "         item = document.getElementById(VMLno[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='hidden';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "     }else{\n");
    gvputs(job, "       for (x in VMLyes){\n");
    gvputs(job, "         item = document.getElementById(VMLyes[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='hidden';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "       for (x in VMLno){\n");
    gvputs(job, "         item = document.getElementById(VMLno[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='visible';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "     }\n");
    gvputs(job, "   }\n");
    gvputs(job, "   </SCRIPT>\n");

    gvputs(job, "</HEAD>");
    gvputs(job, "<BODY onload='browsercheck();'>\n");
    /* add 10pt pad to the bottom of the graph */
    gvputs(job, "<DIV id='_VML1_' style=\"position:relative; display:inline; visibility:hidden");
    gvprintf(job, " width: %upt; height: %upt\">\n", graphWidth, 10+graphHeight);
    gvputs(job, "<STYLE>\n");
    gvputs(job, "v\\:* { behavior: url(#default#VML);display:inline-block}\n"); 
    gvputs(job, "</STYLE>\n"); 
    gvputs(job, "<xml:namespace ns=\"urn:schemas-microsoft-com:vml\" prefix=\"v\" />\n"); 

    gvputs(job, " <v:group style=\"position:relative; ");
    gvprintf(job, " width: %upt; height: %upt\"", graphWidth, graphHeight);
    gvprintf(job, " coordorigin=\"0,0\" coordsize=\"%u,%u\" >", graphWidth, graphHeight);
}

static void vml_end_graph(GVJ_t * job)
{
   gvputs(job, "</v:group>\n");
   gvputs(job, "</DIV>\n");
   /* add 10pt pad to the bottom of the graph */
   gvputs(job, "<DIV id='_VML2_' style=\"position:relative;visibility:hidden\">\n");
   gvputs(job, "<!-- insert any other html content here -->\n");
   gvputs(job, "</DIV>\n");
   gvputs(job, "<DIV id='_notVML1_' style=\"position:relative;\">\n");
   gvputs(job, "<!-- this should only display on NON-IE browsers -->\n");
   gvputs(job, "<H2>Sorry, this diagram will only display correctly on Internet Explorer 5 (and up) browsers.</H2>\n");
   gvputs(job, "</DIV>\n");
   gvputs(job, "<DIV id='_notVML2_' style=\"position:relative;\">\n");
   gvputs(job, "<!-- insert any other NON-IE html content here -->\n");
   gvputs(job, "</DIV>\n");

   gvputs(job, "</BODY>\n</HTML>\n");
}

static void
vml_begin_anchor(GVJ_t * job, char *href, char *tooltip, char *target, char *id)
{
    (void)id;

    gvputs(job, "<a");
    if (href && href[0]) {
	gvputs(job, " href=\"");
	html_puts(job, href);
	gvputs(job, "\"");
    }
    if (tooltip && tooltip[0]) {
	gvputs(job, " title=\"");
	html_puts(job, tooltip);
	gvputs(job, "\"");
    }
    if (target && target[0]) {
	gvputs(job, " target=\"");
	html_puts(job, target);
	gvputs(job, "\"");
    }
    gvputs(job, ">\n");
}

static void vml_end_anchor(GVJ_t * job)
{
    gvputs(job, "</a>\n");
}

static void vml_textspan(GVJ_t * job, pointf p, textspan_t * span)
{
    pointf p1,p2;
    obj_state_t *obj = job->obj;
    PostscriptAlias *pA;

    switch (span->just) {
    case 'l':
	p1.x=p.x;
	break;
    case 'r':
	p1.x=p.x-span->size.x;
	break;
    default:
    case 'n':
	p1.x=p.x-(span->size.x/2);
	break;
    }
    p2.x=p1.x+span->size.x;
    if (span->size.y <  span->font->size){
      span->size.y = 1 + (1.1*span->font->size);
    }

    p1.x-=8; /* vml textbox margin fudge factor */
    p2.x+=8; /* vml textbox margin fudge factor */
    p2.y=graphHeight-(p.y);
    p1.y=(p2.y-span->size.y);
    /* text "y" was too high
     * Graphviz uses "baseline", VML seems to use bottom of descenders - so we fudge a little
     * (heuristics - based on eyeballs)  */
    if (span->font->size <12.){ /*     see graphs/directed/arrows.gv  */
      p1.y+=1.4+span->font->size/5; /* adjust by approx. descender */
      p2.y+=1.4+span->font->size/5; /* adjust by approx. descender */
    }else{
      p1.y+=2+span->font->size/5; /* adjust by approx. descender */
      p2.y+=2+span->font->size/5; /* adjust by approx. descender */
    }

    gvprintf(job, "<v:rect style=\"position:absolute; ");
    gvprintf(job, " left: %.2f; top: %.2f;", p1.x, p1.y);
    gvprintf(job, " width: %.2f; height: %.2f\"", p2.x-p1.x, p2.y-p1.y);
    gvputs(job, " stroked=\"false\" filled=\"false\">\n");
    gvputs(job, "<v:textbox inset=\"0,0,0,0\" style=\"position:absolute; v-text-wrapping:'false';padding:'0';");

    pA = span->font->postscript_alias;
    if (pA) {
        gvprintf(job, "font-family: '%s';", pA->family);
        if (pA->weight)
	    gvprintf(job, "font-weight: %s;", pA->weight);
        if (pA->stretch)
	    gvprintf(job, "font-stretch: %s;", pA->stretch);
        if (pA->style)
	    gvprintf(job, "font-style: %s;", pA->style);
    }
    else {
        gvprintf(job, "font-family: \'%s\';", span->font->name);
    }
    gvprintf(job, " font-size: %.2fpt;", span->font->size);
    switch (obj->pencolor.type) {
    case COLOR_STRING:
	if (strcasecmp(obj->pencolor.u.string, "black"))
	    gvprintf(job, "color:%s;", obj->pencolor.u.string);
	break;
    case RGBA_BYTE:
	gvprintf(job, "color:#%02x%02x%02x;",
		obj->pencolor.u.rgba[0], obj->pencolor.u.rgba[1], obj->pencolor.u.rgba[2]);
	break;
    default:
	UNREACHABLE(); // internal error
    }
    gvputs(job, "\"><center>");
    html_puts(job, span->str);
    gvputs(job, "</center></v:textbox>\n"); 
    gvputs(job, "</v:rect>\n");
}

static void vml_ellipse(GVJ_t * job, pointf * A, int filled)
{   
    double dx, dy, left, top;

    /* A[] contains 2 points: the center and corner. */
    gvputs(job, "  <v:oval style=\"position:absolute;");

    dx=A[1].x-A[0].x;
    dy=A[1].y-A[0].y;

    top=graphHeight-(A[0].y+dy);
    left=A[0].x - dx;
    gvprintf(job, " left: %.2f; top: %.2f;",left, top);
    gvprintf(job, " width: %.2f; height: %.2f\"", 2*dx, 2*dy);

    vml_grfill(job, filled);
    gvputs(job, " >");
    vml_grstroke(job, filled);
    gvputs(job, "</v:oval>\n");
}

static void
vml_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
	      int arrow_at_end, int filled)
{
    (void)arrow_at_start;
    (void)arrow_at_end;

    gvputs(job, " <v:shape style=\"position:absolute; ");
    gvprintf(job, " width: %u; height: %u\"", graphWidth, graphHeight);

    vml_grfill(job, filled);
    gvputs(job, " >");
    vml_grstroke(job, filled);
    gvputs(job, "<v:path  v=\"");
    vml_bzptarray(job, A, n);
    gvputs(job, "/></v:shape>\n");
}

static void vml_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    int i;
    double px,py;

    gvputs(job, " <v:shape style=\"position:absolute; ");
    gvprintf(job, " width: %u; height: %u\"", graphWidth, graphHeight);
    vml_grfill(job, filled);
    gvputs(job, " >");
    vml_grstroke(job, filled);

    gvputs(job, "<v:path  v=\"");
    for (i = 0; i < n; i++)
    {
        px=A[i].x;
        py= (graphHeight-A[i].y);
        if (i==0){
          gvputs(job, "m ");
        }
        /* integers only in path */
	gvprintf(job, "%.0f %.0f ", px, py);
        if (i==0) gvputs(job, "l ");
        if (i==n-1) gvputs(job, "x e \"/>");
    }
    gvputs(job, "</v:shape>\n");
}

static void vml_polyline(GVJ_t * job, pointf * A, int n)
{
    int i;

    gvputs(job, " <v:shape style=\"position:absolute; ");
    gvprintf(job, " width: %u; height: %u\" filled=\"false\">", graphWidth, graphHeight);
    gvputs(job, "<v:path v=\"");
    for (i = 0; i < n; i++)
    {
        if (i==0) gvputs(job, " m ");
	gvprintf(job, "%.0f,%.0f ", A[i].x, graphHeight-A[i].y);
        if (i==0) gvputs(job, " l ");
        if (i==n-1) gvputs(job, " e "); /* no x here for polyline */
    }
    gvputs(job, "\"/>");
    vml_grstroke(job, 0);                 /* no fill here for polyline */
    gvputs(job, "</v:shape>\n");
}

/* color names from 
  http://msdn.microsoft.com/en-us/library/bb250525(VS.85).aspx#t.color
*/
/* NB.  List must be LANG_C sorted */
static char *vml_knowncolors[] = {
     "aqua",  "black",  "blue", "fuchsia", 
     "gray",  "green", "lime",  "maroon", 
     "navy",  "olive",  "purple",  "red",
     "silver",  "teal",  "white",  "yellow"
};

gvrender_engine_t vml_engine = {
    vml_begin_job,
    0,				/* vml_end_job */
    vml_begin_graph,
    vml_end_graph,
    0,                          /* vml_begin_layer */
    0,                          /* vml_end_layer */
    0,                          /* vml_begin_page */
    0,                          /* vml_end_page */
    0,                          /* vml_begin_cluster */
    0,                          /* vml_end_cluster */
    0,				/* vml_begin_nodes */
    0,				/* vml_end_nodes */
    0,				/* vml_begin_edges */
    0,				/* vml_end_edges */
    0,                          /* vml_begin_node */
    0,                          /* vml_end_node */
    0,                          /* vml_begin_edge */
    0,                          /* vml_end_edge */
    vml_begin_anchor,
    vml_end_anchor,
    0,                          /* vml_begin_label */
    0,                          /* vml_end_label */
    vml_textspan,
    0,				/* vml_resolve_color */
    vml_ellipse,
    vml_polygon,
    vml_bezier,
    vml_polyline,
    vml_comment,
    0,				/* vml_library_shape */
};

gvrender_features_t render_features_vml = {
    GVRENDER_Y_GOES_DOWN
        | GVRENDER_DOES_TRANSFORM
	| GVRENDER_DOES_LABELS
	| GVRENDER_DOES_MAPS
	| GVRENDER_DOES_TARGETS
	| GVRENDER_DOES_TOOLTIPS, /* flags */
    0.,                         /* default pad - graph units */
    vml_knowncolors,		/* knowncolors */
    sizeof(vml_knowncolors) / sizeof(char *),	/* sizeof knowncolors */
    RGBA_BYTE,			/* color_type */
};

gvdevice_features_t device_features_vml = {
    GVDEVICE_DOES_TRUECOLOR,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

gvdevice_features_t device_features_vmlz = {
    GVDEVICE_DOES_TRUECOLOR
      | GVDEVICE_COMPRESSED_FORMAT,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

gvplugin_installed_t gvrender_vml_types[] = {
    {FORMAT_VML, "vml", 1, &vml_engine, &render_features_vml},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_vml_types[] = {
    {FORMAT_VML, "vml:vml", 1, NULL, &device_features_vml},
#if HAVE_LIBZ
    {FORMAT_VMLZ, "vmlz:vml", 1, NULL, &device_features_vmlz},
#endif
    {0, NULL, 0, NULL, NULL}
};

