/**
 * @file cr_dsp.h
 * @brief DSP-specific operation handlers for Chronos.
 *
 * This file declares and implements the signal processing functions that manage
 * state (`cr_node::dsp`) for effects, filters, dynamics, and envelopes.
 * These functions are called per-sample during audio processing.
 */
#ifndef CR_DSP_H
#define CR_DSP_H
#include "cr_types.h"

/**
 * @brief State Variable Filter (SVF) handler.
 *
 * Implements Low-Pass, High-Pass, and Band-Pass outputs.
 * @param ctx The current execution context.
 * @param n The node holding filter state (n->dsp.s[0] and n->dsp.s[1]).
 * @param v Array of inputs: v[0]=signal in, v[1]=type (0=LP, 1=HP, 2=BP), v[2]=cutoff (Hz), v[3]=Q/resonance.
 * @return The filtered output signal.
 */
cr_val op_handler_filter(cr_context *ctx, cr_node *n, cr_val *v); 

/**
 * @brief Peaking Equalizer (PEQ) filter handler.
 *
 * Implements a 2nd-order IIR filter used for boosting or cutting a specific frequency band.
 * @param ctx The current execution context.
 * @param n The node holding filter state (n->dsp.s[0] to n->dsp.s[3]).
 * @param v Array of inputs: v[0]=signal in, v[1]=center frequency (Hz), v[2]=gain (dB), v[3]=Q/bandwidth.
 * @return The equalized signal.
 */
cr_val op_handler_peakeq(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Hard Clip distortion handler.
 *
 * Limits the input signal strictly to the range [-1.0, 1.0].
 * @param ctx The current execution context.
 * @param n The node being executed.
 * @param v Array of inputs: v[0]=signal in.
 * @return The clipped signal.
 */
cr_val op_handler_clip(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Hyperbolic Tangent (tanh) soft clipping handler.
 *
 * Applies a smooth distortion/saturation effect.
 * @param ctx The current execution context.
 * @param n The node being executed.
 * @param v Array of inputs: v[0]=signal in.
 * @return The tanh-processed signal.
 */
cr_val op_handler_tanh(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Wavefolding distortion handler.
 *
 * Folds the signal back into range once it exceeds a threshold.
 * @param ctx The current execution context.
 * @param n The node being executed.
 * @param v Array of inputs: v[0]=signal in, v[1]=folding threshold.
 * @return The wavefolded signal.
 */
cr_val op_handler_fold(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Slew Limiter handler.
 *
 * Limits the maximum rate of change of the input signal per sample.
 * Stores the last output value in `n->dsp.s[0]`.
 * @param ctx The current execution context.
 * @param n The node holding the last output state.
 * @param v Array of inputs: v[0]=signal in, v[1]=slew rate (units per second).
 * @return The slew-limited signal.
 */
cr_val op_handler_slew(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Sample and Hold (S/H) handler.
 *
 * Holds the value of the signal input (v[0]) whenever the trigger input (v[1]) crosses a positive threshold (0.5).
 * State variables: `n->dsp.s[0]` holds the held value, `n->dsp.s[1]` holds the last trigger value.
 * @param ctx The current execution context.
 * @param n The node holding the held value and last trigger state.
 * @param v Array of inputs: v[0]=signal to sample, v[1]=trigger signal.
 * @return The held signal value.
 */
cr_val op_handler_sah(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Attack-Decay-Sustain-Release (ADSR) envelope handler.
 *
 * Implements a state machine for envelope generation, typically controlled by a gate signal (phasor in).
 * State variables: `n->dsp.s[0]`=current env value, `n->dsp.s[1]`=stage, `n->dsp.s[4]`=last phasor value.
 * @param ctx The current execution context.
 * @param n The node holding the envelope state.
 * @param v Array of inputs: v[0]=phasor/gate in, v[1]=total duration, v[2]=decay time factor, v[3]=sustain level, v[4]=release time factor.
 * @return The current envelope value (0.0 to 1.0).
 */
cr_val op_handler_adsr(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Delay effect handler.
 *
 * Uses a circular buffer (`n->dsp.buffer`) and cubic interpolation for high-quality fractional delays.
 * @param ctx The current execution context.
 * @param n The node holding the delay buffer and write head.
 * @param v Array of inputs: v[0]=signal in, v[1]=delay time (seconds), v[2]=feedback amount.
 * @return The delayed signal output.
 */
cr_val op_handler_delay(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Reverb effect handler.
 *
 * Implements a complex reverb algorithm using all-pass and comb filters within a large buffer.
 * @param ctx The current execution context.
 * @param n The node holding the reverb tank buffer.
 * @param v Array of inputs: v[0]=signal in, v[1]=decay/RT60, v[2]=damping factor, v[3]=mix level (optional).
 * @return The mixed (wet/dry) signal.
 */
cr_val op_handler_reverb(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Dynamic Range Compressor handler.
 *
 * Uses a smooth envelope follower to dynamically reduce gain for signals exceeding a threshold.
 * @param ctx The current execution context.
 * @param n The node holding the envelope follower state (`n->dsp.s[0]`).
 * @param v Array of inputs: v[0]=signal in, v[1]=threshold (dB), v[2]=ratio.
 * @return The compressed signal.
 */
cr_val op_handler_compress(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Peak Limiter handler.
 *
 * Implements a fast peak detection and release envelope to prevent the signal from exceeding a threshold.
 * @param ctx The current execution context.
 * @param n The node holding peak state (`n->dsp.s[0]`) and gain smoothing state (`n->dsp.s[1]`).
 * @param v Array of inputs: v[0]=signal in, (threshold and release are constants/internally defined).
 * @return The limited signal.
 */
cr_val op_handler_limit(cr_context *ctx, cr_node *n, cr_val *v);

/**
 * @brief Duplicate declaration of op_handler_limit (removed for clean C89 headers).
 */
/* cr_val op_handler_limit(cr_context *ctx, cr_node *n, cr_val *v); */

#ifdef CR_DSP_IMPLEMENTATION
/**
 * @brief State Variable Filter (SVF) handler implementation.
 *
 * Implements Low-Pass, High-Pass, and Band-Pass outputs using the SVF topology.
 *
 * @param ctx The current execution context.
 * @param n The node holding filter state (n->dsp.s[0] and n->dsp.s[1]).
 * @param v Array of inputs: v[0]=signal in, v[1]=type (0=LP, 1=HP, 2=BP), v[2]=cutoff (Hz), v[3]=Q/resonance.
 * @return The filtered output signal.
 */
cr_val op_handler_filter(cr_context *ctx, cr_node *n, cr_val *v) { 
    double in = as_float(v[0]);
    double type = as_float(v[1]);
    double cutoff = as_float(v[2]);
    double q = as_float(v[3]); 
    double f, out_val;
    
    if (q < 0.1) q = 0.1;
    
    f = 2.0 * sin(CR_PI * cutoff / ctx->sample_rate);
    n->dsp.s[0] += f * n->dsp.s[1]; /* lowpass output */
    n->dsp.s[1] += f * (in - n->dsp.s[0] - (1.0/q)*n->dsp.s[1]); /* bandpass output */
    
    if (type < 0.5) out_val = n->dsp.s[1]; /* Band-Pass (based on implementation details) */
    else if (type < 1.5) out_val = n->dsp.s[0]; /* Low-Pass */
    else out_val = in - n->dsp.s[0]; /* High-Pass */
    
    return make_float(out_val);
}

/**
 * @brief Peaking Equalizer (PEQ) filter handler implementation (BiQuad).
 *
 * @param ctx The current execution context.
 * @param n The node holding filter state (n->dsp.s[0] to n->dsp.s[3] for delays).
 * @param v Array of inputs: v[0]=signal in, v[1]=center frequency (Hz), v[2]=gain (dB), v[3]=Q/bandwidth.
 * @return The equalized signal.
 */
cr_val op_handler_peakeq(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double freq = as_float(v[1]);
    double gain_db = as_float(v[2]);
    double q = as_float(v[3]);
    double A, w0, alpha, cos_w0, b0, b1, b2, a0, a1, a2, x1, x2, y1, y2, y0;
    
    /** Design formulas based on BiQuad */
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
    
    /** x1, x2 are input delays, y1, y2 are output delays */
    x1 = n->dsp.s[0]; 
    x2 = n->dsp.s[1];
    y1 = n->dsp.s[2]; 
    y2 = n->dsp.s[3];
    
    /** Direct Form I calculation */
    y0 = (b0/a0)*in + (b1/a0)*x1 + (b2/a0)*x2 - (a1/a0)*y1 - (a2/a0)*y2;
    
    /** Update state variables */
    n->dsp.s[1] = x1; 
    n->dsp.s[0] = in;
    n->dsp.s[3] = y1; 
    n->dsp.s[2] = y0;
    
    return make_float(y0);
}

/**
 * @brief Hard Clip distortion handler implementation.
 *
 * @param ctx The current execution context.
 * @param n The node being executed.
 * @param v Array of inputs: v[0]=signal in.
 * @return The clipped signal.
 */
cr_val op_handler_clip(cr_context *ctx, cr_node *n, cr_val *v) {
    double val = as_float(v[0]);
    if (val > 1.0) val = 1.0; 
    else if (val < -1.0) val = -1.0;
    return make_float(val);
}

/**
 * @brief Hyperbolic Tangent (tanh) soft clipping handler implementation.
 *
 * @param ctx The current execution context.
 * @param n The node being executed.
 * @param v Array of inputs: v[0]=signal in.
 * @return The tanh-processed signal.
 */
cr_val op_handler_tanh(cr_context *ctx, cr_node *n, cr_val *v) {
    return make_float(tanh(as_float(v[0])));
}

/**
 * @brief Wavefolding distortion handler implementation.
 *
 * Folds a signal that exceeds the threshold back toward zero.
 *
 * @param ctx The current execution context.
 * @param n The node being executed.
 * @param v Array of inputs: v[0]=signal in, v[1]=folding threshold.
 * @return The wavefolded signal.
 */
cr_val op_handler_fold(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double thresh = as_float(v[1]);
    if (thresh < 0.001) thresh = 0.001;
    
    in /= thresh;
    
    if (fabs(in) > 1.0) {
        /** Folding logic */
        in = 4.0 * (fabs(0.25 * in + 0.25 - floor(0.25 * in + 0.25)) - 0.25);
    }
    
    return make_float(in * thresh);
}

/**
 * @brief Slew Limiter handler implementation.
 *
 * Limits the rate of change per sample based on the rate input.
 *
 * @param ctx The current execution context.
 * @param n The node holding the last output state (`n->dsp.s[0]`).
 * @param v Array of inputs: v[0]=signal in, v[1]=slew rate (units per second).
 * @return The slew-limited signal.
 */
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

/**
 * @brief Sample and Hold (S/H) handler implementation.
 *
 * Holds the value of input `v[0]` when input `v[1]` (trigger) rises past 0.5.
 *
 * @param ctx The current execution context.
 * @param n The node holding the held value (`n->dsp.s[0]`) and last trigger state (`n->dsp.s[1]`).
 * @param v Array of inputs: v[0]=signal to sample, v[1]=trigger signal.
 * @return The held signal value.
 */
cr_val op_handler_sah(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double trig = as_float(v[1]);
    double last_trig = n->dsp.s[1];
    
    /** Check for positive zero-crossing trigger */
    if (trig > 0.5 && last_trig <= 0.5) n->dsp.s[0] = in;
    
    n->dsp.s[1] = trig;
    return make_float(n->dsp.s[0]);
}

/**
 * @brief Attack-Decay-Sustain-Release (ADSR) envelope handler implementation.
 *
 * Implements a four-stage envelope controlled by a gate/phasor signal.
 * The envelope calculation uses coefficient-based exponential smoothing for
 * Attack, Decay, and Release segments.
 *
 * @param ctx The current execution context.
 * @param n The node holding the envelope state.
 * @param v Array of inputs: v[0]=phasor/gate in, v[1]=total duration, v[2]=decay time factor, v[3]=sustain level, v[4]=release time factor, v[5]=invert phasor, v[6]=retrigger mode.
 * @return The current envelope value (0.0 to 1.0).
 */
cr_val op_handler_adsr(cr_context *ctx, cr_node *n, cr_val *v) { 
     double phasor_in = as_float(v[0]);
     double total_duration = as_float(v[1]);
     double decay_t = as_float(v[2]);
     double sustain_l = as_float(v[3]); 
     double release_t = as_float(v[4]); 
     int mono_mode = (n->input_count > 5) ? as_int(v[5]) : 0;
     int retrigger_mode = (n->input_count > 6) ? as_int(v[6]) : 1;
     int invert_phasor = (n->input_count > 7) ? as_int(v[7]) : as_int(v[5]); /* Recheck indices due to old code pattern */

     double env = n->dsp.s[0];
     int stage = (int)n->dsp.s[1];
     double current_coeff = n->dsp.s[2];
     double target_inv = n->dsp.s[3];
     double last_phasor = n->dsp.s[4];

     double attack_t = total_duration * 0.1; /* Assuming fixed 10% for attack in current script */
     
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
     
     /** Clamp values */
     if (sustain_l < 0.0) sustain_l = 0.0;
     if (sustain_l > 1.0) sustain_l = 1.0;
     if (attack_t < 1e-4) attack_t = 1e-4;
     if (decay_t < 1e-4) decay_t = 1e-4;
     if (release_t < 1e-4) release_t = 1e-4;

     if (should_retrigger) {
         stage = 0; /** Attack stage */
         current_coeff = exp(-1.0 / (attack_t * ctx->sample_rate));
         target_inv = 1.0;
     }
     if (key_up) {
         if (stage != 4) {
             stage = 3; /** Release stage */
             current_coeff = exp(-1.0 / (release_t * ctx->sample_rate));
             target_inv = 0.0;
         }
     }
     
     /** State machine for the four segments */
     if (stage == 0) {
         /** Attack */
         env = target_inv + (env - target_inv) * current_coeff;
         if (env >= 0.9999) {
             env = 1.0;
             stage = 1; /** Transition to Decay */
             current_coeff = exp(-1.0 / (decay_t * ctx->sample_rate));
             target_inv = sustain_l;
         }
     } else if (stage == 1) {
         /** Decay */
         env = target_inv + (env - target_inv) * current_coeff;
         if (env <= sustain_l + 0.0001) {
             env = sustain_l;
             stage = 2; /** Transition to Sustain */
         }
     } else if (stage == 2) {
         /** Sustain */
         env = sustain_l;
     } else if (stage == 3) {
         /** Release */
         env = target_inv + (env - target_inv) * current_coeff;
         if (env <= 0.0001) {
             env = 0.0;
             stage = 4; /** Transition to Off */
         }
     } else {
         /** Off (Stage 4) */
         env = 0.0;
     }
     
     /** Update DSP state */
     n->dsp.s[0] = env;
     n->dsp.s[1] = (double)stage;
     n->dsp.s[2] = current_coeff;
     n->dsp.s[3] = target_inv;
     n->dsp.s[4] = phasor_in;
     return make_float(env);
}

/**
 * @brief Delay effect handler implementation.
 *
 * Uses cubic interpolation for smooth, time-varying delay.
 *
 * @param ctx The current execution context.
 * @param n The node holding the delay buffer (`n->dsp.buffer`).
 * @param v Array of inputs: v[0]=signal in, v[1]=delay time, v[2]=feedback.
 * @return The delayed signal output.
 */
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
        
        /** Calculate read position (current write head minus delay in samples) */
        read_pos = n->dsp.write_head - delay_samples;
        while (read_pos < 0) read_pos += n->dsp.buf_len;
        
        /** Interpolation setup: 4 points (i0 to i3) */
        i_part = (int)read_pos;
        f_part = read_pos - i_part; /* Fractional part for interpolation */
        
        /** Calculate indices and wrap around the circular buffer */
        i0 = i_part - 1; if (i0 < 0) i0 += n->dsp.buf_len;
        i1 = i_part;
        i2 = i_part + 1; if (i2 >= n->dsp.buf_len) i2 -= n->dsp.buf_len;
        i3 = i_part + 2; if (i3 >= n->dsp.buf_len) i3 -= n->dsp.buf_len;
        
        y0 = n->dsp.buffer[i0];
        y1 = n->dsp.buffer[i1];
        y2 = n->dsp.buffer[i2];
        y3 = n->dsp.buffer[i3];
        
        /** Cubic interpolation (Horner's method for efficiency) */
        c0 = y1;
        c1 = 0.5 * (y2 - y0);
        c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
        
        delayed = ((c3 * f_part + c2) * f_part + c1) * f_part + c0;
        
        /** Calculate value to write (Input + Delayed Signal * Feedback) */
        write_val = in + (delayed * fb);
        
        /** Apply soft clipping to the feedback path to prevent runaway oscillation */
        soft = tanh(write_val * 0.5); 
        
        /** Write the feedback-mixed signal to the buffer */
        n->dsp.buffer[n->dsp.write_head] = soft;
        n->dsp.write_head = (n->dsp.write_head + 1) % n->dsp.buf_len;
    } else delayed = in;
    
    return make_float(delayed);
}

/**
 * @brief Reverb effect handler implementation.
 *
 * Implements a complex feedback delay network (FDN) structure, likely based on
 * a classic digital reverb design (e.g., Freeverb or similar comb/all-pass topology).
 *
 * @param ctx The current execution context.
 * @param n The node holding the reverb tank buffer.
 * @param v Array of inputs: v[0]=signal in, v[1]=decay, v[2]=damping.
 * @return The mixed (wet/dry) signal.
 */
cr_val op_handler_reverb(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double decay = as_float(v[1]);
    double damping = as_float(v[2]);
    double mix = (n->input_count > 3) ? as_float(v[3]) : 0.5; /** Note: The input is v[3] not dereferencing ctx->node_pool here */
    int mask, wh;
    double sr_scale, bw, bw_state, in_filt, t0, k1, k2, k3, lfo;
    int pre_delay;
    /** Delays for the initial schroeder all-pass section */
    int d1, d2, d3, d4;
    /** Delays for the feedback comb filter sections (d5-d11) */
    int d5, d6, d7, d9, d10, d11; 
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
        sr_scale = ctx->sample_rate / 29761.0; /** Scaling factor relative to a fixed sample rate */
        
        /** Delay lengths are scaled by sr_scale */
        pre_delay = (int)(0.020 * ctx->sample_rate);
        d1 = (int)(142.0 * sr_scale); d2 = (int)(379.0 * sr_scale);
        d3 = (int)(902.0 * sr_scale); d4 = (int)(287.0 * sr_scale);
        
        /** Input damping/filtering */
        bw = 0.999; 
        bw_state = n->dsp.s[2];
        in_filt = in * (1.0 - bw) + bw_state * bw;
        n->dsp.s[2] = in_filt;
        
        pd_read = (wh - pre_delay) & mask;
        n->dsp.buffer[wh] = in_filt;
        t0 = n->dsp.buffer[pd_read];
        
        /** First stage all-pass network (Schroeder style) */
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
        
        /** Comb filter delay lengths */
        d5 = (int)(4217.0 * sr_scale); d6 = (int)(2656.0 * sr_scale);
        d7 = (int)(4453.0 * sr_scale); d9 = (int)(4453.0 * sr_scale); d10 = (int)(1800.0 * sr_scale);
        d11 = (int)(3720.0 * sr_scale); 
        
        /** Stereo cross-feedback input (from the other channel's output) */
        l_in = t4_out + n->dsp.s[4]; 
        r_in = t4_out + n->dsp.s[3]; 
        
        /** LFO for subtle delay time modulation (chorus/flutter) */
        lfo = sin(n->dsp.s[1]);
        n->dsp.s[1] += 0.001; 
        
        /** --- Left Channel FDN Loop --- */
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
        
        /** Damping filter (LP filter in the feedback loop) */
        l_damp = l3_out * (1.0 - damping) + n->dsp.s[5] * damping;
        n->dsp.s[5] = l_damp;
        n->dsp.buffer[l3_r] = l3_in; 
        n->dsp.s[3] = l_damp; /** Cross-feedback for Right Channel (n->dsp.s[3]) */

        /** --- Right Channel FDN Loop --- */
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
        n->dsp.s[4] = r_damp; /** Cross-feedback for Left Channel (n->dsp.s[4]) */
        
        /** Stereo output mix (cross-feed reads from various delay taps) */
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
        
        /** Final wet/dry mix */
        return make_float(in * (1.0 - mix) + wet_sig * mix);
    } 
    return make_float(in);
}

/**
 * @brief Dynamic Range Compressor handler implementation.
 *
 * Uses a feed-forward topology with separate attack and release envelope followers.
 *
 * @param ctx The current execution context.
 * @param n The node holding the envelope follower state (`n->dsp.s[0]`).
 * @param v Array of inputs: v[0]=signal in, v[1]=threshold (dB), v[2]=ratio.
 * @return The compressed signal.
 */
cr_val op_handler_compress(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double thresh_db = as_float(v[1]);
    double ratio = as_float(v[2]);
    double attack = 0.010; /** Fixed 10ms attack */
    double release = 0.100; /** Fixed 100ms release */
    double alpha_att, alpha_rel, abs_in, env, env_db, gain_db, over_db, gain;
    
    if (ratio < 1.0) ratio = 1.0;
    
    /** Calculate single-pole filter coefficients for smoothing envelope */
    alpha_att = exp(-1.0 / (attack * ctx->sample_rate));
    alpha_rel = exp(-1.0 / (release * ctx->sample_rate));
    
    abs_in = fabs(in);
    env = n->dsp.s[0];
    
    /** Envelope Follower (Fast Attack, Slow Release behavior) */
    if (abs_in > env) env = alpha_att * env + (1.0 - alpha_att) * abs_in;
    else env = alpha_rel * env + (1.0 - alpha_rel) * abs_in;
    n->dsp.s[0] = env;
    
    /** Convert envelope level to dB */
    env_db = (env > 1e-6) ? 20.0 * log10(env) : -120.0;
    gain_db = 0.0;
    
    if (env_db > thresh_db) {
        /** Calculate amount of compression needed */
        over_db = env_db - thresh_db;
        gain_db = -over_db * (1.0 - 1.0 / ratio);
    }
    
    /** Convert gain back to linear scale and apply to input signal */
    gain = pow(10.0, gain_db / 20.0);
    return make_float(in * gain);
}

/**
 * @brief Peak Limiter handler implementation.
 *
 * Uses a simplified dynamic gain reduction applied after peak detection.
 *
 * @param ctx The current execution context.
 * @param n The node holding peak state (`n->dsp.s[0]`) and gain smoothing state (`n->dsp.s[1]`).
 * @param v Array of inputs: v[0]=signal in.
 * @return The limited signal.
 */
cr_val op_handler_limit(cr_context *ctx, cr_node *n, cr_val *v) {
    double in = as_float(v[0]);
    double threshold = 0.95; /** Fixed threshold */
    double release = 0.050; /** Fixed 50ms release */
    double alpha_rel, abs_in, peak, gain, gain_smooth;
    
    /** Release coefficient (simple exponential smoothing) */
    alpha_rel = exp(-1.0 / (release * ctx->sample_rate));
    abs_in = fabs(in);
    peak = n->dsp.s[0];
    
    /** Peak detection (fastest attack possible: max of current input or smoothed previous peak) */
    if (abs_in > peak) peak = abs_in; 
    else peak = alpha_rel * peak + (1.0 - alpha_rel) * abs_in;
    n->dsp.s[0] = peak;
    
    gain = 1.0;
    if (peak > threshold) gain = threshold / peak; /** Instantaneous gain reduction based on peak */
    
    /** Smooth the gain change */
    gain_smooth = n->dsp.s[1];
    if (gain < gain_smooth) gain_smooth = gain; /** Instantaneous gain reduction (fastest attack) */
    else gain_smooth = alpha_rel * gain_smooth + (1.0 - alpha_rel) * gain; /** Exponential release */
    n->dsp.s[1] = gain_smooth;
    
    return make_float(in * gain_smooth);
}

#endif
#endif
