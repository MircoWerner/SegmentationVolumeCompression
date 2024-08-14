#ifndef PATHTRACE_GLSL
#define PATHTRACE_GLSL

vec3 pathtraceSingle(inout uint rngState, in vec3 origin, in vec3 direction) {
    vec3 throughput = vec3(1);
    vec3 color = vec3(0);

    HitPayload payload;
    vec3 position;
    vec3 normal;
    Material material;

    for (int bounce = 0; bounce < g_bounces; bounce++) {
        if (!trace(origin, direction, 0.001, 100000.0, payload)) {
            color += throughput * environment_evaluate(direction);
            break;
        }

        intersectionInfo(payload, origin, direction, position, normal, material);

        if (material_isLightSource(material)) {
            color += throughput * material.emission;
            //            break;
        }

        const BSDFVertex vertex = BSDFVertex(normal);
        const BSDFFrame frame = coordinateSystem(normal);

        const float xiLobe = nextFloat(rngState);
        const float xi1X2 = nextFloat(rngState);
        const float xi2X2 = nextFloat(rngState);
        vec3 rayOutgoing;
        if (!bsdf_disney_sample(vertex, frame, material, -direction, rayOutgoing, bsdf_disney_lobe_index(material, xiLobe), vec2(xi1X2, xi2X2))) {
            //        if (!bsdf_disney_clearcoat_sample(vertex, frame, material, -direction, rayOutgoing, vec2(xi1X2, xi2X2))) {
            //        if (!bsdf_disney_sheen_sample(vertex, frame, material, -direction, rayOutgoing, vec2(xi1X2, xi2X2))) {
            //        if (!bsdf_disney_metal_sample(vertex, frame, material, -direction, rayOutgoing, vec2(xi1X2, xi2X2))) {
            //        if (!bsdf_disney_diffuse_sample(vertex, frame, material, -direction, rayOutgoing, vec2(xi1X2, xi2X2))) {
            return vec3(0);
        }
        const float pdf = bsdf_disney_pdf(vertex, frame, material, -direction, rayOutgoing);
        //        const float pdf = bsdf_disney_clearcoat_pdf(vertex, frame, material, -direction, rayOutgoing);
        //        const float pdf = bsdf_disney_sheen_pdf(vertex, frame, material, -direction, rayOutgoing);
        //        const float pdf = bsdf_disney_metal_pdf(vertex, frame, material, -direction, rayOutgoing);
        //        const float pdf = bsdf_disney_diffuse_pdf(vertex, frame, material, -direction, rayOutgoing);
        if (pdf <= 0) {
            return vec3(1, 1, 0);// indicate error
        }
        const vec3 f = bsdf_disney_evaluate(vertex, frame, material, -direction, rayOutgoing);
        //        const vec3 f = bsdf_disney_clearcoat_evaluate(vertex, frame, material, -direction, rayOutgoing);
        //        const vec3 f = bsdf_disney_sheen_evaluate(vertex, frame, material, -direction, rayOutgoing);
        //        const vec3 f = bsdf_disney_metal_evaluate(vertex, frame, material, -direction, rayOutgoing);
        //        const vec3 f = bsdf_disney_diffuse_evaluate(vertex, frame, material, -direction, rayOutgoing);

        throughput *= f / pdf;

        origin = position + 0.001 * normal;
        direction = rayOutgoing;

        const float pRoulette = max(throughput.r, max(throughput.g, throughput.b));
        if (nextFloat(rngState) > pRoulette) {
            break;
        }
        throughput /= pRoulette;
    }

    return color;
}

vec3 pathtrace(inout uint rngState, in const ivec2 pixel) {
    // load direction
    vec3 direction;
    {
        uint pixelState = (g_frame + 1) * g_pixels_x * (pixel.y + 1) + pixel.x;
        float offsetX = nextFloat(pixelState) - 0.5;// [-0.5,0.5]
        float offsetY = nextFloat(pixelState) - 0.5;// [-0.5,0.5]

        vec2 pixel_uv = (vec2(pixel.xy) + vec2(0.5) + vec2(offsetX, offsetY)) / vec2(g_pixels_x, g_pixels_y);
        direction = normalize(mix(
        mix(vec3(g_ray_left_bottom_x, g_ray_left_bottom_y, g_ray_left_bottom_z), vec3(g_ray_left_top_x, g_ray_left_top_y, g_ray_left_top_z), pixel_uv.y),
        mix(vec3(g_ray_right_bottom_x, g_ray_right_bottom_y, g_ray_right_bottom_z), vec3(g_ray_right_top_x, g_ray_right_top_y, g_ray_right_top_z), pixel_uv.y),
        pixel_uv.x));
    }

    if (g_num_instances == 0) {
        return environment_evaluate(direction);
    }

    // trace
    vec3 color = vec3(0);
    for (int i = 0; i < g_spp; i++) {
        color += pathtraceSingle(rngState, vec3(g_ray_origin_x, g_ray_origin_y, g_ray_origin_z), direction);
    }
    color /= float(g_spp);

    return color;
}

#endif