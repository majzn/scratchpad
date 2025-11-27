#ifndef CR_DSP_H
#define CR_DSP_H
#include "cr_types.h"

cr_val op_handler_filter(cr_context *ctx, cr_node *n, cr_val *v); 
cr_val op_handler_peakeq(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_clip(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_tanh(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_fold(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_slew(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_sah(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_adsr(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_delay(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_reverb(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_compress(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_limit(cr_context *ctx, cr_node *n, cr_val *v);
cr_val op_handler_limit(cr_context *ctx, cr_node *n, cr_val *v);

#ifdef CR_DSP_IMPLEMENTATION
cr_val op_handler_filter(cr_context *ctx, cr_node *n, cr_val *v) { 
    double in = as_float(v[0]);
    double type = as_float(v[1]);
    double cutoff = as_float(v[2]);
    double q = as_float(v[3]); 
    double f, out_val;
    
    if (q < 0.1) q = 0.1;
    
    f = 2.0 * sin(CR_PI * cutoff / ctx->sample_rate);
    n->dsp.s[0] += f * n->dsp.s[1];
    n->dsp.s[1] += f * (in - n->dsp.s[0] - (1.0/q)*n->dsp.s[1]);
    
    if (type < 0.5) out_val = n->dsp.s[1]; 
    else if (type < 1.5) out_val = n->dsp.s[0]; 
    else out_val = in - n->dsp.s[0]; 
    
    return make_float(out_val);
}

cr_val op_handler_peakeq(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double freq = as_float(v[1]);
    double gain_db = as_float(v[2]);
    double q = as_float(v[3]);
    double A, w0, alpha, cos_w0, b0, b1, b2, a0, a1, a2, x1, x2, y1, y2, y0;
    
    A = pow(10.0, gain_db / 40.0);
    w0 = 2.0 * CR_PI * freq / ctx->sample_rate;
    alpha = sin(w0) / (2.0 * q);
    cos_w0 = cos(w0);
    b0 = 1.0 + alpha * A;
    b1 = -2.0 * cos_w0;
    b2 = 1.0 - alpha * A;
    a0 = 1.0 + alpha / A;
    a1 = -2.0 * cos_w0;
    a2 = 1.0 - alpha / A;
    x1 = n->dsp.s[0]; x2 = n->dsp.s[1];
    y1 = n->dsp.s[2]; y2 = n->dsp.s[3];
    y0 = (b0/a0)*in + (b1/a0)*x1 + (b2/a0)*x2 - (a1/a0)*y1 - (a2/a0)*y2;
    n->dsp.s[1] = x1; n->dsp.s[0] = in;
    n->dsp.s[3] = y1; n->dsp.s[2] = y0;
    return make_float(y0);
}

cr_val op_handler_clip(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    if (val > 1.0) val = 1.0; else if (val < -1.0) val = -1.0;
    return make_float(val);
}

cr_val op_handler_tanh(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(tanh(as_float(v[0])));
}

cr_val op_handler_fold(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double thresh = as_float(v[1]);
    if (thresh < 0.001) thresh = 0.001;
    in /= thresh;
    if (fabs(in) > 1.0) {
        in = 4.0 * (fabs(0.25 * in + 0.25 - floor(0.25 * in + 0.25)) - 0.25);
    }
    return make_float(in * thresh);
}

cr_val op_handler_slew(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double rate = as_float(v[1]);
    double current = n->dsp.s[0];
    double max_change = rate / ctx->sample_rate;
    if (in > current + max_change) current += max_change;
    else if (in < current - max_change) current -= max_change;
    else current = in;
    n->dsp.s[0] = current;
    return make_float(current);
}

cr_val op_handler_sah(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double trig = as_float(v[1]);
    double last_trig = n->dsp.s[1];
    if (trig > 0.5 && last_trig <= 0.5) n->dsp.s[0] = in;
    n->dsp.s[1] = trig;
    return make_float(n->dsp.s[0]);
}

cr_val op_handler_adsr(cr_context *ctx, cr_node *n, cr_val *v) { 
     double phasor_in = as_float(v[0]);
     double total_duration = as_float(v[1]);
     double decay_t = as_float(v[2]);
     double sustain_l = as_float(v[3]); 
     double release_t = as_float(v[4]); 
     int mono_mode = (n->input_count > 5) ? as_int(v[5]) : 0;
     int retrigger_mode = (n->input_count > 6) ? as_int(v[6]) : 1;
     double env = n->dsp.s[0];
     int stage = (int)n->dsp.s[1];
     double current_coeff = n->dsp.s[2];
     double target_inv = n->dsp.s[3];
     double last_phasor = n->dsp.s[4];
		 int invert_phasor = as_int(v[5]);

     double attack_t = total_duration * 0.1;
     
     double key_down = invert_phasor ? (phasor_in < 0.1) : (phasor_in > 0.1);
     
     double release_start_ratio = (total_duration - release_t) / total_duration;
     
     double internal_gate_on = (phasor_in < release_start_ratio);
     double key_up = (!internal_gate_on && last_phasor < release_start_ratio);
     
     int should_retrigger = key_down;
     
     if (mono_mode == 1 && internal_gate_on) {
         if (retrigger_mode == 0) {
             if (stage != 4) should_retrigger = 0;
         }
     }
     if (sustain_l < 0.0) sustain_l = 0.0;
     if (sustain_l > 1.0) sustain_l = 1.0;
     if (attack_t < 1e-4) attack_t = 1e-4;
     if (decay_t < 1e-4) decay_t = 1e-4;
     if (release_t < 1e-4) release_t = 1e-4;

     if (should_retrigger) {
         stage = 0;
         current_coeff = exp(-1.0 / (attack_t * ctx->sample_rate));
         target_inv = 1.0;
     }
     if (key_up) {
         if (stage != 4) {
             stage = 3;
             current_coeff = exp(-1.0 / (release_t * ctx->sample_rate));
             target_inv = 0.0;
         }
     }
     if (stage == 0) {
         env = target_inv + (env - target_inv) * current_coeff;
         if (env >= 0.9999) {
             env = 1.0;
             stage = 1;
             current_coeff = exp(-1.0 / (decay_t * ctx->sample_rate));
             target_inv = sustain_l;
         }
     } else if (stage == 1) {
         env = target_inv + (env - target_inv) * current_coeff;
         if (env <= sustain_l + 0.0001) {
             env = sustain_l;
             stage = 2;
         }
     } else if (stage == 2) {
         env = sustain_l;
     } else if (stage == 3) {
         env = target_inv + (env - target_inv) * current_coeff;
         if (env <= 0.0001) {
             env = 0.0;
             stage = 4;
         }
     } else {
         env = 0.0;
     }
     n->dsp.s[0] = env;
     n->dsp.s[1] = (double)stage;
     n->dsp.s[2] = current_coeff;
     n->dsp.s[3] = target_inv;
     n->dsp.s[4] = phasor_in;
     return make_float(env);
}

cr_val op_handler_delay(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double time = as_float(v[1]);
    double fb = as_float(v[2]);
    double delayed = 0.0;
    double delay_samples;
    double read_pos;
    int i_part, i0, i1, i2, i3;
    double f_part, y0, y1, y2, y3, c0, c1, c2, c3, write_val, soft;

    if (n->dsp.buffer && n->dsp.buf_len > 0) {
        delay_samples = time * ctx->sample_rate;
        if (delay_samples < 1.0) delay_samples = 1.0;
        if (delay_samples > n->dsp.buf_len - 4) delay_samples = n->dsp.buf_len - 4;
        
        read_pos = n->dsp.write_head - delay_samples;
        while (read_pos < 0) read_pos += n->dsp.buf_len;
        
        i_part = (int)read_pos;
        f_part = read_pos - i_part;
        
        i0 = i_part - 1; if (i0 < 0) i0 += n->dsp.buf_len;
        i1 = i_part;
        i2 = i_part + 1; if (i2 >= n->dsp.buf_len) i2 -= n->dsp.buf_len;
        i3 = i_part + 2; if (i3 >= n->dsp.buf_len) i3 -= n->dsp.buf_len;
        
        y0 = n->dsp.buffer[i0];
        y1 = n->dsp.buffer[i1];
        y2 = n->dsp.buffer[i2];
        y3 = n->dsp.buffer[i3];
        
        c0 = y1;
        c1 = 0.5 * (y2 - y0);
        c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
        
        delayed = ((c3 * f_part + c2) * f_part + c1) * f_part + c0;
        
        write_val = in + (delayed * fb);
        soft = tanh(write_val * 0.5); 
        
        n->dsp.buffer[n->dsp.write_head] = soft;
        n->dsp.write_head = (n->dsp.write_head + 1) % n->dsp.buf_len;
    } else delayed = in;
    
    return make_float(delayed);
}

cr_val op_handler_reverb(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double decay = as_float(v[1]);
    double damping = as_float(v[2]);
    double mix = (n->input_count > 3) ? as_float(ctx->node_pool[n->inputs[3]].value) : 0.5;
    int mask, wh;
    double sr_scale, bw, bw_state, in_filt, t0, k1, k2, k3, lfo;
    int pre_delay, d1, d2, d3, d4, d5, d6, d7, d9, d10, d11;
    int pd_read, t1_r, t2_r, t3_r, t4_r;
    double t1_out, t2_out, t3_out, t4_out, l_in, r_in;
    int d5_len_i, l1_r, l2_r, l3_r;
    double l1_out, l_diff_in, l2_out, l3_in, l3_out, l_damp;
    int d9_len_i, r1_r, r2_r, r3_r;
    double r1_out, r_diff_in, r2_out, r3_in, r3_out, r_damp;
    double out_l, out_r, wet_sig;
    
    if (decay > 0.999) decay = 0.999;
    if (damping > 0.99) damping = 0.99;
    
    if (n->dsp.buffer && n->dsp.buf_len > 0) {
        mask = n->dsp.buf_len - 1;
        wh = n->dsp.write_head;
        sr_scale = ctx->sample_rate / 29761.0;
        
        pre_delay = (int)(0.020 * ctx->sample_rate);
        d1 = (int)(142.0 * sr_scale); d2 = (int)(379.0 * sr_scale);
        d3 = (int)(902.0 * sr_scale); d4 = (int)(287.0 * sr_scale);
        
        bw = 0.999; 
        bw_state = n->dsp.s[2];
        in_filt = in * (1.0 - bw) + bw_state * bw;
        n->dsp.s[2] = in_filt;
        
        pd_read = (wh - pre_delay) & mask;
        n->dsp.buffer[wh] = in_filt;
        t0 = n->dsp.buffer[pd_read];
        
        k1 = 0.75; k2 = 0.625; k3 = 0.5;
        
        t1_r = (wh - pre_delay - d1) & mask;
        t1_out = n->dsp.buffer[t1_r] - k1 * t0;
        n->dsp.buffer[t1_r] = t0 + k1 * t1_out;
        
        t2_r = (wh - pre_delay - d1 - d2) & mask;
        t2_out = n->dsp.buffer[t2_r] - k2 * t1_out;
        n->dsp.buffer[t2_r] = t1_out + k2 * t2_out;
        
        t3_r = (wh - pre_delay - d1 - d2 - d3) & mask;
        t3_out = n->dsp.buffer[t3_r] - k2 * t2_out;
        n->dsp.buffer[t3_r] = t2_out + k2 * t3_out;
        
        t4_r = (wh - pre_delay - d1 - d2 - d3 - d4) & mask;
        t4_out = n->dsp.buffer[t4_r] - k2 * t3_out;
        n->dsp.buffer[t4_r] = t3_out + k2 * t4_out;
        
        d5 = (int)(4217.0 * sr_scale); d6 = (int)(2656.0 * sr_scale);
        d7 = (int)(4453.0 * sr_scale); d9 = (int)(4453.0 * sr_scale); d10 = (int)(1800.0 * sr_scale);
        d11 = (int)(3720.0 * sr_scale); 
        
        l_in = t4_out + n->dsp.s[4]; 
        r_in = t4_out + n->dsp.s[3]; 
        
        lfo = sin(n->dsp.s[1]);
        n->dsp.s[1] += 0.001; 
        
        d5_len_i = d5 + (int)(lfo * 8.0);
        l1_r = (wh - d5_len_i) & mask;
        l1_out = n->dsp.buffer[l1_r];
        n->dsp.buffer[l1_r] = l_in; 
        
        l_diff_in = l1_out;
        l2_r = (wh - d5_len_i - d6) & mask;
        l2_out = n->dsp.buffer[l2_r] - k3 * l_diff_in;
        n->dsp.buffer[l2_r] = l_diff_in + k3 * l2_out;
        
        l3_in = l2_out * decay;
        l3_r = (wh - d5_len_i - d6 - d7) & mask;
        l3_out = n->dsp.buffer[l3_r];
        
        l_damp = l3_out * (1.0 - damping) + n->dsp.s[5] * damping;
        n->dsp.s[5] = l_damp;
        n->dsp.buffer[l3_r] = l3_in; 
        n->dsp.s[3] = l_damp; 

        d9_len_i = d9 - (int)(lfo * 8.0);
        r1_r = (wh - d9_len_i) & mask;
        r1_out = n->dsp.buffer[r1_r];
        n->dsp.buffer[r1_r] = r_in;
        
        r_diff_in = r1_out;
        r2_r = (wh - d9_len_i - d10) & mask;
        r2_out = n->dsp.buffer[r2_r] - k3 * r_diff_in;
        n->dsp.buffer[r2_r] = r_diff_in + k3 * r2_out;
        
        r3_in = r2_out * decay;
        r3_r = (wh - d9_len_i - d10 - d11) & mask;
        r3_out = n->dsp.buffer[r3_r];
        
        r_damp = r3_out * (1.0 - damping) + n->dsp.s[6] * damping;
        n->dsp.s[6] = r_damp;
        n->dsp.buffer[r3_r] = r3_in;
        n->dsp.s[4] = r_damp; 
        
        out_l = 0.6 * n->dsp.buffer[(wh - d5 - d6 - 200) & mask] 
                     + 0.6 * n->dsp.buffer[(wh - d5 - d6 - d7 - 200) & mask]
                     - 0.6 * n->dsp.buffer[(wh - d9 - d10 - 200) & mask]
                     - 0.6 * n->dsp.buffer[(wh - d9 - d10 - d11 - 200) & mask];
                     
        out_r = 0.6 * n->dsp.buffer[(wh - d9 - d10 - 200) & mask] 
                     + 0.6 * n->dsp.buffer[(wh - d9 - d10 - d11 - 200) & mask]
                     - 0.6 * n->dsp.buffer[(wh - d5 - d6 - 200) & mask]
                     - 0.6 * n->dsp.buffer[(wh - d5 - d6 - d7 - 200) & mask];
        
        n->dsp.write_head = (wh + 1) & mask;
        wet_sig = (out_l + out_r) * 0.5;
        return make_float(in * (1.0 - mix) + wet_sig * mix);
    } 
    return make_float(in);
}

cr_val op_handler_compress(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double thresh_db = as_float(v[1]);
    double ratio = as_float(v[2]);
    double attack = 0.010; 
    double release = 0.100;
    double alpha_att, alpha_rel, abs_in, env, env_db, gain_db, over_db, gain;
    
    if (ratio < 1.0) ratio = 1.0;
    
    alpha_att = exp(-1.0 / (attack * ctx->sample_rate));
    alpha_rel = exp(-1.0 / (release * ctx->sample_rate));
    abs_in = fabs(in);
    env = n->dsp.s[0];
    
    if (abs_in > env) env = alpha_att * env + (1.0 - alpha_att) * abs_in;
    else env = alpha_rel * env + (1.0 - alpha_rel) * abs_in;
    n->dsp.s[0] = env;
    
    env_db = (env > 1e-6) ? 20.0 * log10(env) : -120.0;
    gain_db = 0.0;
    
    if (env_db > thresh_db) {
        over_db = env_db - thresh_db;
        gain_db = -over_db * (1.0 - 1.0 / ratio);
    }
    gain = pow(10.0, gain_db / 20.0);
    return make_float(in * gain);
}

cr_val op_handler_limit(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double threshold = 0.95;
    double release = 0.050;
    double alpha_rel, abs_in, peak, gain, gain_smooth;
    
    alpha_rel = exp(-1.0 / (release * ctx->sample_rate));
    abs_in = fabs(in);
    peak = n->dsp.s[0];
    
    if (abs_in > peak) peak = abs_in; 
    else peak = alpha_rel * peak + (1.0 - alpha_rel) * abs_in;
    n->dsp.s[0] = peak;
    
    gain = 1.0;
    if (peak > threshold) gain = threshold / peak;
    
    gain_smooth = n->dsp.s[1];
    if (gain < gain_smooth) gain_smooth = gain; 
    else gain_smooth = alpha_rel * gain_smooth + (1.0 - alpha_rel) * gain; 
    n->dsp.s[1] = gain_smooth;
    return make_float(in * gain_smooth);
}

#endif
#endif
