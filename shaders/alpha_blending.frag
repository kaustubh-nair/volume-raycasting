/*
 * Copyright © 2018 Martino Pilia <martino.pilia@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#version 130

out vec4 a_colour;

uniform mat4 ViewMatrix;
uniform mat3 NormalMatrix;

uniform float focal_length;
uniform float aspect_ratio;
uniform vec2 viewport_size;
uniform vec3 ray_origin;
uniform vec3 top;
uniform vec3 bottom;

uniform vec3 background_colour;
uniform vec3 material_colour;
uniform vec3 light_position;

uniform float step_length;
uniform float threshold;
uniform float transfer_function_threshold;
uniform float hsv_tf_h_threshold;
uniform float hsv_tf_s_threshold;
uniform float hsv_tf_v_threshold;

uniform sampler3D volume;
uniform sampler2D jitter;
uniform sampler3D color_proximity_tf;
uniform sampler3D space_proximity_tf;

uniform float gamma;

// Ray
struct Ray {
    vec3 origin;
    vec3 direction;
};

// Axis-aligned bounding box
struct AABB {
    vec3 top;
    vec3 bottom;
};

vec3 rgb2hsv(vec3 rgb)
{
    vec3 hsv;
    float min_val, max_val, delta;

    min_val = (rgb.r < rgb.g) ? rgb.r : rgb.g;
    min_val = (min_val  < rgb.b) ? min_val  : rgb.b;

    max_val = (rgb.r > rgb.g) ? rgb.r : rgb.g;
    max_val = (max_val  > rgb.b) ? max_val  : rgb.b;

    hsv.z = max_val;                                // v
    delta = max_val - min_val;
    if (delta < 0.00001)
    {
        hsv.y = 0;
        hsv.x = 0; // undefrgbed, maybe nan?
        return hsv;
    }
    if( max_val > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        hsv.y = (delta / max_val);                  // s
    } else {
        // if max_val is 0, then r = g = b = 0              
        // s = 0, h is undefrgbed
        hsv.y = 0.0;
        hsv.x = 0.0;
        return hsv;
    }
    if( rgb.r >= max_val )                           // > is bogus, just keeps compilor happy
        hsv.x = ( rgb.g - rgb.b ) / delta;        // between yellow & magenta
    else
    if( rgb.g >= max_val )
        hsv.x = 2.0 + ( rgb.b - rgb.r ) / delta;  // between cyan & yellow
    else
        hsv.x = 4.0 + ( rgb.r - rgb.g ) / delta;  // between magenta & cyan

    hsv.x *= 60.0;                              // degrees

    if( hsv.x < 0.0 )
        hsv.x += 360.0;

	hsv.x = hsv.x/360.0;  // convert 0 ,1

    return hsv;
}

// Estimate normal from a finite difference approximation of the gradient
vec3 normal(vec3 position, float intensity)
{
    float d = step_length;
    float dx = texture(volume, position + vec3(d,0,0)).r - intensity;
    float dy = texture(volume, position + vec3(0,d,0)).r - intensity;
    float dz = texture(volume, position + vec3(0,0,d)).r - intensity;
    return -normalize(NormalMatrix * vec3(dx, dy, dz));
}

// Slab method for ray-box intersection
void ray_box_intersection(Ray ray, AABB box, out float t_0, out float t_1)
{
    vec3 direction_inv = 1.0 / ray.direction;
    vec3 t_top = direction_inv * (box.top - ray.origin);
    vec3 t_bottom = direction_inv * (box.bottom - ray.origin);
    vec3 t_min = min(t_top, t_bottom);
    vec2 t = max(t_min.xx, t_min.yz);
    t_0 = max(0.0, max(t.x, t.y));
    vec3 t_max = max(t_top, t_bottom);
    t = min(t_max.xx, t_max.yz);
    t_1 = min(t.x, t.y);
}

// A very simple colour transfer function
vec4 colour_transfer(float intensity)
{
    vec3 high = vec3(1.0, 1.0, 1.0);
    vec3 low = vec3(0.0, 0.0, 0.0);
    float alpha = (exp(intensity) - 1.0) / (exp(1.0) - 1.0);
    return vec4(intensity * high + (1.0 - intensity) * low, alpha);
}

void main()
{
    vec3 ray_direction;
    ray_direction.xy = 2.0 * gl_FragCoord.xy / viewport_size - 1.0;
    ray_direction.x *= aspect_ratio;
    ray_direction.z = -focal_length;
    ray_direction = (vec4(ray_direction, 0) * ViewMatrix).xyz;

    float t_0, t_1;
    Ray casting_ray = Ray(ray_origin, ray_direction);
    AABB bounding_box = AABB(top, bottom);
    ray_box_intersection(casting_ray, bounding_box, t_0, t_1);

    vec3 ray_start = (ray_origin + ray_direction * t_0 - bottom) / (top - bottom);
    vec3 ray_stop = (ray_origin + ray_direction * t_1 - bottom) / (top - bottom);

    vec3 ray = ray_stop - ray_start;
    float ray_length = length(ray);
    vec3 step_vector = step_length * ray / ray_length;

    // Random jitter
    ray_start += step_vector;

    vec3 position = ray_start;
    vec4 colour = vec4(0.0);

    a_colour = vec4(position,1.0);

    // Ray march until reaching the end of the volume, or colour saturation
    while (ray_length > 0 && colour.a < 1.0) {

        vec4 c = texture(volume, position).gbar;

        if (c.x > transfer_function_threshold && c.y > transfer_function_threshold && c.z > transfer_function_threshold)
            c = vec4(0.0);
		else
        {
            vec3 hsv_value = rgb2hsv(c.xyz);
            if (hsv_value.x > hsv_tf_h_threshold && hsv_value.y > hsv_tf_s_threshold && hsv_value.z > hsv_tf_v_threshold)
                c = vec4(0.0);
        }
        c.a = texture(color_proximity_tf, c.rgb).r;
        float a = texture(space_proximity_tf, position).r;
        if (a < c.a)
            c.a = a;


        // enable this for single channel datasets
        //float intensity = texture(volume, position).r;
        //vec4 c = colour_transfer(intensity);

        // Alpha-blending
        colour.rgb = c.a * c.rgb + (1 - c.a) * colour.a * colour.rgb;
        colour.a = c.a + (1 - c.a) * colour.a;

        ray_length -= step_length;
        position += step_vector;
    }

    // Blend background
    colour.rgb = colour.a * colour.rgb + (1 - colour.a) * pow(background_colour, vec3(gamma)).rgb;
    colour.a = 1.0;

    // Gamma correction
    a_colour.rgb = pow(colour.rgb, vec3(1.0 / gamma));
    a_colour.a = texture(color_proximity_tf, colour.rgb).r;
}
