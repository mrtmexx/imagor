#include "foreign.h"

typedef int (*SetSaveOptionsFn)(VipsOperation *operation, SaveParams *params);

int save_target(const char *operationName, SaveParams *params,
                SetSaveOptionsFn setSaveOptions) {
  VipsOperation *operation = vips_operation_new(operationName);
  if (!operation) {
    return 1;
  }

  if (vips_object_set(VIPS_OBJECT(operation), "in", params->inputImage, NULL)) {
    return 1;
  }
  if (vips_object_set(VIPS_OBJECT(operation), "target", (VipsTarget*) params->target, NULL)) {
    return 1;
  }

  if (setSaveOptions(operation, params)) {
    g_object_unref(operation);
    return 1;
  }

  if (vips_cache_operation_buildp(&operation)) {
    vips_object_unref_outputs(VIPS_OBJECT(operation));
    g_object_unref(operation);
    return 1;
  }

  return 0;
}

// https://libvips.github.io/libvips/API/current/VipsForeignSave.html#vips-jpegsave-target
int set_jpegsave_options(VipsOperation *operation, SaveParams *params) {
  int ret = vips_object_set(
      VIPS_OBJECT(operation), "strip", params->stripMetadata, "optimize_coding",
      params->jpegOptimizeCoding, "interlace", params->interlace,
      "subsample_mode", params->jpegSubsample, "trellis_quant",
      params->jpegTrellisQuant, "overshoot_deringing",
      params->jpegOvershootDeringing, "optimize_scans",
      params->jpegOptimizeScans, "quant_table", params->jpegQuantTable, NULL);

  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  return ret;
}

// https://libvips.github.io/libvips/API/current/VipsForeignSave.html#vips-pngsave-target
int set_pngsave_options(VipsOperation *operation, SaveParams *params) {
  int ret =
      vips_object_set(VIPS_OBJECT(operation), "strip", params->stripMetadata,
                      "compression", params->pngCompression, "interlace",
                      params->interlace, "filter", params->pngFilter, "palette",
                      params->pngPalette, NULL);

  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  if (!ret && params->pngDither) {
    ret = vips_object_set(VIPS_OBJECT(operation), "dither", params->pngDither, NULL);
  }

  if (!ret && params->pngBitdepth) {
    ret = vips_object_set(VIPS_OBJECT(operation), "bitdepth", params->pngBitdepth, NULL);
  }

  // TODO: Handle `profile` param.

  return ret;
}

// https://github.com/libvips/libvips/blob/master/libvips/foreign/webpsave.c#L524
// https://libvips.github.io/libvips/API/current/VipsForeignSave.html#vips-webpsave-target
int set_webpsave_options(VipsOperation *operation, SaveParams *params) {
  int ret =
      vips_object_set(VIPS_OBJECT(operation),
                      "strip", params->stripMetadata,
                      "lossless", params->webpLossless,
                      "near_lossless", params->webpNearLossless,
                      "reduction_effort", params->webpReductionEffort,
                      "profile", params->webpIccProfile ? params->webpIccProfile : "none",
                      NULL);

  if (!ret && params->quality) {
    vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  return ret;
}

// https://github.com/libvips/libvips/blob/master/libvips/foreign/heifsave.c#L653
int set_heifsave_options(VipsOperation *operation, SaveParams *params) {
  int ret = vips_object_set(VIPS_OBJECT(operation), "lossless",
                            params->heifLossless, NULL);

  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  return ret;
}

// https://libvips.github.io/libvips/API/current/VipsForeignSave.html#vips-tiffsave-target
int set_tiffsave_options(VipsOperation *operation, SaveParams *params) {
  int ret = vips_object_set(
      VIPS_OBJECT(operation), "strip", params->stripMetadata, "compression",
      params->tiffCompression, "predictor", params->tiffPredictor, "pyramid",
      params->tiffPyramid, "tile_height", params->tiffTileHeight, "tile_width",
      params->tiffTileWidth, "tile", params->tiffTile, "xres", params->tiffXRes,
      "yres", params->tiffYRes, NULL);

  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  return ret;
}

// https://libvips.github.io/libvips/API/current/VipsForeignSave.html#vips-magicksave-target
int set_magicksave_options(VipsOperation *operation, SaveParams *params) {
  int ret = vips_object_set(VIPS_OBJECT(operation), "format", "GIF", NULL);
  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "quality", params->quality,
                          NULL);
  }
  return ret;
}

// https://libvips.github.io/libvips/API/current/VipsForeignSave.html#vips-gifsave-target
int set_gifsave_options(VipsOperation *operation, SaveParams *params) {
  int ret = 0;
  // See for argument values: https://www.libvips.org/API/current/VipsForeignSave.html#vips-gifsave
  if (params->gifDither > 0.0 && params->gifDither <= 1.0) {
    ret = vips_object_set(VIPS_OBJECT(operation), "dither", params->gifDither, NULL);
  }
  if (params->gifEffort >= 1 && params->gifEffort <= 10) {
    ret = vips_object_set(VIPS_OBJECT(operation), "effort", params->gifEffort, NULL);
  }
  if (params->gifBitdepth >= 1 && params->gifBitdepth <= 8) {
      ret = vips_object_set(VIPS_OBJECT(operation), "bitdepth", params->gifBitdepth, NULL);
  }
  return ret;
}

int set_avifsave_options(VipsOperation *operation, SaveParams *params) {
  int ret = vips_object_set(
      VIPS_OBJECT(operation), "compression", VIPS_FOREIGN_HEIF_COMPRESSION_AV1,
      "lossless", params->heifLossless, "speed", params->avifSpeed, NULL);

  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  return ret;
}

int set_jp2ksave_options(VipsOperation *operation, SaveParams *params) {
  int ret = vips_object_set(
      VIPS_OBJECT(operation), "subsample_mode", params->jpegSubsample,
      "tile_height", params->jp2kTileHeight, "tile_width", params->jp2kTileWidth,
      "lossless", params->jp2kLossless, NULL);

  if (!ret && params->quality) {
    ret = vips_object_set(VIPS_OBJECT(operation), "Q", params->quality, NULL);
  }

  return ret;
}

int save_to_target(SaveParams *params) {
  switch (params->outputFormat) {
    case JPEG:
      return save_target("jpegsave_target", params, set_jpegsave_options);
    case PNG:
      return save_target("pngsave_target", params, set_pngsave_options);
    case WEBP:
      return save_target("webpsave_target", params, set_webpsave_options);
    case HEIF:
      return save_target("heifsave_target", params, set_heifsave_options);
    case TIFF:
      return save_target("tiffsave_target", params, set_tiffsave_options);
    case GIF:
#if (VIPS_MAJOR_VERSION >= 8) && (VIPS_MINOR_VERSION >= 12)
      return save_target("gifsave_target", params, set_gifsave_options);
#else
      return save_target("magicksave_target", params, set_magicksave_options);
#endif
    case AVIF:
      return save_target("heifsave_target", params, set_avifsave_options);
    case JP2K:
      return save_target("jp2ksave_target", params, set_jp2ksave_options);
    default:
      g_warning("Unsupported output type given: %d", params->outputFormat);
  }
  return 1;
}

static SaveParams defaultSaveParams = {
    .inputImage = NULL,
    .target = NULL,
    .outputFormat = JPEG,

    .interlace = FALSE,
    .quality = 0,
    .stripMetadata = FALSE,

    .jpegOptimizeCoding = FALSE,
    .jpegSubsample = VIPS_FOREIGN_JPEG_SUBSAMPLE_ON,
    .jpegTrellisQuant = FALSE,
    .jpegOvershootDeringing = FALSE,
    .jpegOptimizeScans = FALSE,
    .jpegQuantTable = 0,

    .pngCompression = 6,
    .pngPalette = FALSE,
    .pngBitdepth = 0,
    .pngDither = 0,
    .pngFilter = VIPS_FOREIGN_PNG_FILTER_NONE,

    .gifDither = 0.0,
    .gifEffort = 0,
    .gifBitdepth = 0,

    .webpLossless = FALSE,
    .webpNearLossless = FALSE,
    .webpReductionEffort = 4,
    .webpIccProfile = NULL,

    .heifLossless = FALSE,

    .tiffCompression = VIPS_FOREIGN_TIFF_COMPRESSION_LZW,
    .tiffPredictor = VIPS_FOREIGN_TIFF_PREDICTOR_HORIZONTAL,
    .tiffPyramid = FALSE,
    .tiffTile = FALSE,
    .tiffTileHeight = 256,
    .tiffTileWidth = 256,
    .tiffXRes = 1.0,
    .tiffYRes = 1.0,

    .avifSpeed = 5,

    .jp2kLossless = FALSE,
    .jp2kTileHeight = 512,
    .jp2kTileWidth = 512};

SaveParams create_save_params(ImageType outputFormat) {
  SaveParams params = defaultSaveParams;
  params.outputFormat = outputFormat;
  return params;
}
