#!/usr/bin/env python
# coding: utf-8

import argparse
import os
import subprocess
import base64
import json

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import tensorflow as tf
import tensorflow_probability as tfp
from tensorflow.keras import (
    backend,
    initializers,
    layers,
    models,
    regularizers,
)
from tensorflow.keras import mixed_precision

# mixed_precision.set_global_policy("mixed_float16")
# mixed_precision.set_global_policy("mixed_bfloat16")
# tf.config.optimizer.set_jit(False)

# Use legacy Keras if needed
os.environ["TF_USE_LEGACY_KERAS"] = "1"


# ----------------------------
# GPU setup
# ----------------------------
physical_devices = tf.config.list_physical_devices("GPU")
print(physical_devices, flush=True)
if physical_devices:
    try:
        tf.config.experimental.set_memory_growth(physical_devices[0], True)
    except Exception as e:  # pragma: no cover
        print(f"Could not set memory growth: {e}", flush=True)


# ----------------------------
# Argument parsing
# ----------------------------
def parse_args():
    parser = argparse.ArgumentParser(
        description="TensorFlow Neural Network Training Script"
    )

    parser.add_argument(
        "--npatterns",
        type=int,
        required=True,
        help="Number of patterns.",
    )
    parser.add_argument(
        "--nmaterials",
        type=int,
        required=True,
        choices=[0, 2, 4, 5, 6, 7, 8, 9],
        help="Number of material patterns.",
    )
    parser.add_argument(
        "--data",
        type=str,
        required=True,
        help="Dataset.",
    )
    parser.add_argument(
        "--architecture_patterns",
        type=str,
        choices=["null",  "add", "concat"],
        default="concat",
        help="Architecture patterns: (default: concat).",
    )
    parser.add_argument(
        "--architecture_material",
        type=str,
        choices=["null", "add", "concat"],
        default="concat",
        help="Architecture material: (default: concat).",
    )
    parser.add_argument(
        "--nembed_pattern",
        type=int,
        default=32,
        help="Size of pattern embedding vector (default: 32).",
    )
    parser.add_argument(
        "--nembed_material",
        type=int,
        default=32,
        help="Size of material embedding vector (default: 32).",
    )
    parser.add_argument(
        "--exponent",
        type=int,
        default=3,
        help="Exponent regularizer (default: 3).",
    )
    parser.add_argument(
        "--nlayer0_pattern",
        type=int,
        default=16,
        help="Number of neurons in the first layer (default: 16).",
    )
    parser.add_argument(
        "--nlayer1_pattern",
        type=int,
        default=0,
        help="Number of neurons in the second layer (default: 0).",
    )
    parser.add_argument(
        "--nlayer2_pattern",
        type=int,
        default=0,
        help="Number of neurons in the third layer (default: 0).",
    )
    parser.add_argument(
        "--nlayer0_material",
        type=int,
        default=16,
        help="Number of neurons in the first layer (default: 16).",
    )
    parser.add_argument(
        "--nlayer1_material",
        type=int,
        default=0,
        help="Number of neurons in the second layer (default: 0).",
    )
    parser.add_argument(
        "--nlayer2_material",
        type=int,
        default=0,
        help="Number of neurons in the third layer (default: 0).",
    )
    parser.add_argument(
        "--nlayer0",
        type=int,
        default=16,
        help="Number of neurons in the first layer (default: 16).",
    )
    parser.add_argument(
        "--nlayer1",
        type=int,
        default=0,
        help="Number of neurons in the second layer (default: 0).",
    )
    parser.add_argument(
        "--nlayer2",
        type=int,
        default=0,
        help="Number of neurons in the third layer (default: 0).",
    )
    parser.add_argument(
        "--activation_inputs",
        type=str,
        choices=["relu", "rsqrt", "geluv2"],
        default="relu",
        help="Activation inputs (default: relu).",
    )
    parser.add_argument(
        "--activation",
        type=str,
        choices=["relu", "rsqrt", "geluv2"],
        default="relu",
        help="Activation layer (default: relu).",
    )
    parser.add_argument(
        "--activation_last",
        type=str,
        choices=["linear", "tanh", "logits"],
        default="linear",
        help="Activation layer (default: linear).",
    )
    parser.add_argument(
        "--use_bias",
        type=int,
        choices=[0, 1],
        default=1,
        help="Use bias: 1=True, 0=False (default: 1).",
    )
    parser.add_argument(
        "--loss",
        type=str,
        choices=["huber", "mse", "sccentropy"],
        default="huber",
        help="Loss (default: huber).",
    )
    parser.add_argument(
        "--save",
        type=int,
        choices=[0, 1],
        default=1,
        help="Save intermediate artifacts (default: 1).",
    )
    parser.add_argument(
        "--batch_size",
        type=int,
        default=65536,
        help="Batch size (default: 65536).",
    )
    parser.add_argument(
        "--nepochs",
        type=int,
        default=1001,
        help="Number of epochs (default: 1001).",
    )
    parser.add_argument(
        "--patience",
        type=int,
        default=2,
        help="Patience (default: 2).",
    )
    parser.add_argument(
        "--interval",
        type=int,
        default=10,
        help="Percentile calculation interval (default: 10).",
    )
    parser.add_argument(
        "--tag",
        type=str,
        default="",
        help="tag (default: empty).",
    )

    return parser.parse_args()


# ----------------------------
# Config / derived params
# ----------------------------
args = parse_args()

if args.architecture_patterns == "null":
    args.npatterns = 0
    args.nlayer0_pattern = 0
    args.nlayer1_pattern = 0
    args.nlayer2_pattern = 0

if args.architecture_material == "null":
    args.nmaterials = 0
    args.nlayer0_material = 0
    args.nlayer1_material = 0
    args.nlayer2_material = 0

NPATTERNS = args.npatterns
NMATERIALS = args.nmaterials
DATA = args.data
ARCHITECTURE_PATTERNS = args.architecture_patterns
ARCHITECTURE_MATERIAL = args.architecture_material
NEMBED_PATTERN = args.nembed_pattern
NEMBED_MATERIAL = args.nembed_material
EXPONENT = args.exponent
NLAYER0_PATTERN = args.nlayer0_pattern
NLAYER1_PATTERN = args.nlayer1_pattern
NLAYER2_PATTERN = args.nlayer2_pattern
NLAYER0_MATERIAL = args.nlayer0_material
NLAYER1_MATERIAL = args.nlayer1_material
NLAYER2_MATERIAL = args.nlayer2_material
NLAYER0 = args.nlayer0
NLAYER1 = args.nlayer1
NLAYER2 = args.nlayer2
ACTIVATION_INPUTS = args.activation_inputs
ACTIVATION = args.activation
ACTIVATION_LAST = args.activation_last
USE_BIAS = bool(args.use_bias)
LOSS_NAME = args.loss
BATCH_SIZE = args.batch_size
NEPOCHS = args.nepochs
PATIENCE = args.patience
INTERVAL = args.interval
SAVE = args.save
TAG = args.tag

# layer0 + layer1 + output layer
# output layer is hard-coded to dense3
NLAYERS = 4

ROOT = "/tmp5/gwies"

# ----------------------------
# Utilities
# ----------------------------
def query_compute_apps():
    print("GPU MEMORY USAGE")
    result = subprocess.run(
        ["nvidia-smi", "--query-compute-apps=pid,used_memory", "--format=csv"],
        stdout=subprocess.PIPE,
        check=False,
    )
    print(result.stdout.decode("utf-8"))


if NMATERIALS in (2, 4, 5, 6, 7, 8, 9):
    MATERIAL_NP_TYPE = np.uint32
else:
    MATERIAL_NP_TYPE = np.uint8

MATERIAL_NP_SIZE = np.dtype(MATERIAL_NP_TYPE).itemsize

if ACTIVATION_LAST == "logits":
    LABEL_NP_DTYPE = np.int32
    LABEL_TF_DTYPE = tf.int32
else:
    LABEL_NP_DTYPE = np.float32
    LABEL_TF_DTYPE = tf.float32

def count_records(raw: np.ndarray):
    record_bytes = NPATTERNS * 4 + NMATERIALS * MATERIAL_NP_SIZE + 4 

    if raw.nbytes % record_bytes != 0:
        raise ValueError(
            f"Array size {raw.nbytes} not divisible by record size {record_bytes}"
        )

    return raw.nbytes // record_bytes

def label_stats(raw: np.ndarray, pattern_embed_sizes):
    record_bytes = NPATTERNS * 4 + NMATERIALS * MATERIAL_NP_SIZE + 4

    nrecords = raw.shape[0] // record_bytes
    records = raw[:nrecords * record_bytes].reshape((nrecords, record_bytes))

    labels = records[:, -4:].copy().view(np.float32).reshape(-1)

    print(
        f"Label stats: "
        f"n={labels.size} "
        f"mean={labels.mean():.9f} "
        f"std={labels.std():.9f} "
        f"min={labels.min():.9f} "
        f"max={labels.max():.9f}"
    )

    nnegative = np.count_nonzero(labels < 0.0)
    nzero     = np.count_nonzero(labels == 0.0)
    npositive = np.count_nonzero(labels > 0.0)

    print(
        f"Label signs: "
        f"negative={nnegative} ({100.0 * nnegative / labels.size:.4f}%) "
        f"zero={nzero} ({100.0 * nzero / labels.size:.4f}%) "
        f"positive={npositive} ({100.0 * npositive / labels.size:.4f}%)"
    )

def count_pattern_occurrences(raw: np.ndarray, pattern_embed_sizes):
    record_bytes = NPATTERNS * 4 + NMATERIALS * MATERIAL_NP_SIZE + 4

    nrecords = raw.shape[0] // record_bytes
    records = raw[:nrecords * record_bytes].reshape((nrecords, record_bytes))

    offset_material = NPATTERNS * 4
    patterns = records[:, :offset_material].view(np.uint32)

    assert patterns.shape == (nrecords, NPATTERNS)

    counts = []

    for p in range(NPATTERNS):
        assert patterns[:, p].max() < pattern_embed_sizes[p]

        c = np.bincount(
            patterns[:, p],
            minlength=int(pattern_embed_sizes[p])
        )
        counts.append(c)

    return counts

def make_compact_pattern_maps(pattern_counts):
    """Build original-state -> compact-state maps for pattern embeddings.

    Seen states are mapped densely to compact ids 0..N-1. States that never
    occur in the training set are marked with UINT32_MAX. They are filtered
    out of the validation set before batches are generated and therefore are
    never passed to an Embedding layer. Original state 0 is treated exactly
    like every other valid pattern state.
    """
    pattern_state_maps = []
    compact_pattern_embed_sizes = np.empty(NPATTERNS, dtype=np.uint32)
    invalid = np.iinfo(np.uint32).max

    for p, counts in enumerate(pattern_counts):
        seen_indices = np.flatnonzero(counts != 0)
        nseen = len(seen_indices)

        mapping = np.full(len(counts), invalid, dtype=np.uint32)
        mapping[seen_indices] = np.arange(nseen, dtype=np.uint32)

        pattern_state_maps.append(mapping)
        compact_pattern_embed_sizes[p] = nseen

        print(
            f"pattern {p}: compacting {len(counts)} -> {nseen} rows "
            f"({100.0 * nseen / len(counts):.2f}% of original)"
        )

    return pattern_state_maps, compact_pattern_embed_sizes


def make_validation_seen_mask(raw: np.ndarray, pattern_counts):
    """Return a mask selecting validation records fully covered by training.

    A validation record is kept only when every pattern occupation in that
    record occurred at least once in the training set.
    """
    record_bytes = NPATTERNS * 4 + NMATERIALS * MATERIAL_NP_SIZE + 4
    nrecords = raw.shape[0] // record_bytes
    records = raw[:nrecords * record_bytes].reshape((nrecords, record_bytes))

    offset_material = NPATTERNS * 4
    patterns = records[:, :offset_material].view(np.uint32)
    assert patterns.shape == (nrecords, NPATTERNS)

    keep = np.ones(nrecords, dtype=bool)

    for p in range(NPATTERNS):
        states = patterns[:, p]
        counts = pattern_counts[p]

        if states.max() >= len(counts):
            raise ValueError(
                f"validation pattern {p}: state id exceeds original embedding size"
            )

        seen_here = counts[states] != 0
        nunseen = int(np.count_nonzero(~seen_here))

        print(
            f"validation pattern {p}: {nunseen} / {nrecords} records "
            f"({100.0 * nunseen / nrecords:.6f}%) use a state unseen in training"
        )

        keep &= seen_here

    nkeep = int(np.count_nonzero(keep))
    nremoved = nrecords - nkeep

    print(
        f"validation: removing {nremoved} / {nrecords} records "
        f"({100.0 * nremoved / nrecords:.6f}%) containing at least one "
        f"pattern state unseen in training"
    )
    print(f"validation: keeping {nkeep} records")

    return keep


def count_material_occurrences(raw: np.ndarray, material_embed_sizes):
    record_bytes = NPATTERNS * 4 + NMATERIALS * MATERIAL_NP_SIZE + 4

    nrecords = raw.shape[0] // record_bytes
    records = raw[:nrecords * record_bytes].reshape((nrecords, record_bytes))

    offset_material = NPATTERNS * 4
    offset_label = offset_material + NMATERIALS * MATERIAL_NP_SIZE

    materials = records[:, offset_material:offset_label].view(MATERIAL_NP_TYPE)

    assert materials.shape == (nrecords, NMATERIALS)

    counts = []

    for m in range(NMATERIALS):
        assert materials[:, m].max() < material_embed_sizes[m]

        c = np.bincount(
            materials[:, m],
            minlength=int(material_embed_sizes[m])
        )
        counts.append(c)

    return counts

def raw_binary_batch_generator(
    raw: np.ndarray,
    batch_size: int,
    pattern_state_maps,
    record_mask=None,
):
    record_bytes = NPATTERNS * 4 + NMATERIALS * MATERIAL_NP_SIZE + 4

    total_bytes = raw.shape[0]
    nrecords = total_bytes // record_bytes
    records = raw.reshape((nrecords, record_bytes))

    offset_patterns = 0
    offset_material = NPATTERNS * 4
    offset_label = offset_material + NMATERIALS * MATERIAL_NP_SIZE
    invalid = np.iinfo(np.uint32).max

    if record_mask is not None:
        if len(record_mask) != nrecords:
            raise ValueError(
                f"record mask has {len(record_mask)} entries, expected {nrecords}"
            )

        # Pack only selected records into full batches without making a second
        # full copy of the validation data.
        chunk_records = max(batch_size * 8, batch_size)
        pending = None

        def selected_record_chunks():
            nonlocal pending
            for start in range(0, nrecords, chunk_records):
                stop = min(start + chunk_records, nrecords)
                selected = records[start:stop][record_mask[start:stop]]
                if selected.shape[0] == 0:
                    continue

                if pending is not None and pending.shape[0] != 0:
                    selected = np.concatenate((pending, selected), axis=0)
                    pending = None

                nfull = (selected.shape[0] // batch_size) * batch_size
                for i in range(0, nfull, batch_size):
                    yield selected[i:i + batch_size]

                pending = selected[nfull:]
    else:
        def selected_record_chunks():
            for i in range(0, nrecords, batch_size):
                j = i + batch_size
                if j > nrecords:
                    break
                yield records[i:j]

    for batch_buf in selected_record_chunks():
        original_patterns = (
            batch_buf[:, offset_patterns:offset_material]
            .view(np.uint32)
            .reshape((batch_size, NPATTERNS))
        )

        patterns = np.empty_like(original_patterns, dtype=np.uint32)
        for p in range(NPATTERNS):
            mapping = pattern_state_maps[p]
            if original_patterns[:, p].max() >= len(mapping):
                raise ValueError(
                    f"pattern {p}: state id exceeds original embedding size"
                )

            compact = mapping[original_patterns[:, p]]
            if np.any(compact == invalid):
                bad = int(original_patterns[:, p][compact == invalid][0])
                raise ValueError(
                    f"pattern {p}: state {bad} was not seen in the training set"
                )

            patterns[:, p] = compact

        material = batch_buf[:, offset_material:offset_label].view(MATERIAL_NP_TYPE)
        labels = batch_buf[:, -4:].view(LABEL_NP_DTYPE).flatten()

        yield (patterns, material), labels

def prepare_datasets():
    training_bin = f"{ROOT}/{DATA}-training.bin"
    validation_bin = f"{ROOT}/{DATA}-validation.bin"

    with open(training_bin, "rb") as ftraining_bin:
        pattern_embed_sizes = np.fromfile(ftraining_bin, dtype=np.uint32, count=NPATTERNS)
        print("pattern_embed_sizes:", pattern_embed_sizes)

        material_embed_sizes = np.fromfile(ftraining_bin, dtype=np.uint32, count=NMATERIALS)
        print("material_embed_sizes:", material_embed_sizes)

        training_data = np.fromfile(ftraining_bin, dtype=np.uint8)

        label_stats(training_data, pattern_embed_sizes)

        pattern_counts = count_pattern_occurrences(training_data, pattern_embed_sizes)
        pattern_state_maps, compact_pattern_embed_sizes = make_compact_pattern_maps(
            pattern_counts
        )

        material_counts = count_material_occurrences(training_data, material_embed_sizes)

    #validation_data = training_data.copy()
    validation_data = np.fromfile(validation_bin, dtype=np.uint8)

    label_stats(validation_data, pattern_embed_sizes)

    validation_seen_mask = make_validation_seen_mask(
        validation_data, pattern_counts
    )

    return (
        pattern_embed_sizes,
        compact_pattern_embed_sizes,
        pattern_state_maps,
        material_embed_sizes,
        training_data,
        validation_data,
        validation_seen_mask,
        pattern_counts,
        material_counts,
    )

def return_activation(activation, x):
    if activation == "relu":
        return tf.keras.activations.relu(x)
    elif activation == "tanh":
        return tf.keras.activations.tanh(x)
    elif activation == "rsqrt":
        if mixed_precision.global_policy().name in ("mixed_float16", "mixed_bfloat16"):
            x32 = tf.keras.ops.cast(x, tf.float32)
            one32 = tf.keras.ops.cast(1.0, tf.float32)
            y32 = x32 * tf.keras.ops.rsqrt(one32 + tf.keras.ops.square(x32))
            return tf.keras.ops.cast(y32, x.dtype)
        else:
            return x * tf.keras.ops.rsqrt(1.0 + tf.keras.ops.square(x))
    elif activation == "gelu":
        return tf.keras.activations.gelu(x)
    elif activation == "geluv2":
        if mixed_precision.global_policy().name in ("mixed_float16", "mixed_bfloat16"):
            x32 = tf.keras.ops.cast(x, tf.float32)
            y32 = 0.5 * x32 * (1.0 + x32 * tf.keras.ops.rsqrt(0.544790 + tf.keras.ops.square(x32)))
            return tf.keras.ops.cast(y32, x.dtype)
        else:
            return 0.5 * x * (1.0 + x * tf.keras.ops.rsqrt(0.544790 + tf.keras.ops.square(x)))
    elif activation == "linear":
        return tf.keras.activations.linear(x)
    else:
        raise ValueError(f"Unknown activation: {ACTIVATION}")


def print_wdl_scalar_diagnostics(scalar_predictions, scalar_labels):
    for target, name in [(-1.0, "lost"), (0.0, "draw"), (1.0, "won")]:
        mask = scalar_labels == target
        p = scalar_predictions[mask]

        print()
        print(f"{name}:")
        print(f"  n      : {len(p)}")
        print(f"  mean   : {np.mean(p): .8f}")
        print(f"  median : {np.median(p): .8f}")
        print(f"  std    : {np.std(p): .8f}")
        print(f"  min    : {np.min(p): .8f}")
        print(f"  max    : {np.max(p): .8f}")
        print(f"  p01    : {np.percentile(p, 1): .8f}")
        print(f"  p05    : {np.percentile(p, 5): .8f}")
        print(f"  p10    : {np.percentile(p, 10): .8f}")
        print(f"  p25    : {np.percentile(p, 25): .8f}")
        print(f"  p50    : {np.percentile(p, 50): .8f}")
        print(f"  p75    : {np.percentile(p, 75): .8f}")
        print(f"  p90    : {np.percentile(p, 90): .8f}")
        print(f"  p95    : {np.percentile(p, 95): .8f}")
        print(f"  p99    : {np.percentile(p, 99): .8f}")

    lost = scalar_predictions[scalar_labels == -1.0]
    draw = scalar_predictions[scalar_labels ==  0.0]
    won  = scalar_predictions[scalar_labels ==  1.0]

    print()
    print("ordering:")
    print(f"  mean   lost/draw/won: {np.mean(lost): .8f} {np.mean(draw): .8f} {np.mean(won): .8f}")
    print(f"  median lost/draw/won: {np.median(lost): .8f} {np.median(draw): .8f} {np.median(won): .8f}")
    print()
    print(f"  lost < 0: {100.0 * np.mean(lost < 0.0): .4f}%")
    print(f"  draw < 0: {100.0 * np.mean(draw < 0.0): .4f}%")
    print(f"  won  > 0: {100.0 * np.mean(won  > 0.0): .4f}%")


def array_to_json_file(name, array, encoding):
    """
    Convert a one- or two-dimensional NumPy array into one entry for the
    JSON 'files' array.
    """
    if encoding == "base64-float32":
        array = np.ascontiguousarray(array, dtype=np.float32)
    elif encoding == "base64-int32":
        array = np.ascontiguousarray(array, dtype=np.int32)
    else:
        raise ValueError(f"Unsupported encoding: {encoding}")

    if array.ndim == 1:
        array = array.reshape(-1, 1)

    if array.ndim != 2:
        raise ValueError(
            f"{name}: expected a 1D or 2D array, got shape {array.shape}"
        )

    rows, cols = array.shape

    return {
        "name": name,
        "encoding": encoding,
        "rows": int(rows),
        "cols": int(cols),
        "data": base64.b64encode(array.tobytes(order="C")).decode("ascii"),
    }

# ----------------------------
# Callbacks
# ----------------------------
class EpochEnd(tf.keras.callbacks.Callback):
    def __init__(self, validation_dataset, model_dir, interval=10):
        super().__init__()
        self.validation_dataset = validation_dataset
        self.model_dir = model_dir
        self.interval = interval
        if not os.path.exists(self.model_dir):
            os.makedirs(self.model_dir)

    def on_epoch_end(self, epoch, logs=None):
        if (epoch + 1) % self.interval != 0:
            return

        # Save layer weights/biases (if requested)

        if SAVE:
            val_loss = logs.get("val_loss")
        
            save_order = (
                [f"pattern_embedding{i}" for i in range(NPATTERNS)]
                + [f"pattern_dense{i}" for i in range(NLAYERS)]
                + [f"material_embedding{i}" for i in range(NMATERIALS)]
                + [f"material_dense{i}" for i in range(NLAYERS)]
                + [f"final_dense{i}" for i in range(NLAYERS)]
            )
        
            print(f"save_order: {save_order}")
        
            names = {layer.name for layer in self.model.layers}
        
            print(f"names: {names}")
        
            files = []
        
            for name in save_order:
                if name not in names:
                    print(f"skipping missing layer: {name}")
                    continue
        
                print(f"name: {name}")
        
                layer = self.model.get_layer(name)
                weights_biases = layer.get_weights()
        
                if not weights_biases:
                    print(f"skipping layer without weights: {name}")
                    continue
        
                if len(weights_biases) == 2:
                    weights, biases = weights_biases
                elif len(weights_biases) == 1:
                    weights = weights_biases[0]
                    biases = np.zeros(
                        (weights.shape[-1],),
                        dtype=np.float32,
                    )
                else:
                    raise ValueError(
                        f"{name}: unexpected number of weight arrays: "
                        f"{len(weights_biases)}"
                    )
        
                # Always work on copies. In particular, zeroing unseen material
                # embeddings must not modify the model's actual weights.
                weights = np.asarray(weights, dtype=np.float32).copy()
                biases = np.asarray(biases, dtype=np.float32).copy()
        
                #
                # Pattern embeddings are trained in compact form. Seen states
                # occupy rows 0..N-1. Validation records containing unseen states
                # are filtered out, so every compact row corresponds to a state
                # observed in training.
                #
                if name.startswith("pattern_embedding"):
                    ipattern = int(name[len("pattern_embedding"):])
        
                    seen_indices = np.flatnonzero(
                        pattern_counts[ipattern] != 0
                    ).astype(np.int32)
        
                    assert weights.shape[0] == len(seen_indices), (
                        f"{name}: {weights.shape[0]} embedding rows, but "
                        f"{len(seen_indices)} seen indices"
                    )
        
                    index_name = f"{name}_index.csv"
        
                    files.append(
                        array_to_json_file(
                            index_name,
                            seen_indices,
                            "base64-int32",
                        )
                    )
        
                    print(
                        f"pattern {ipattern}: saving {len(seen_indices)} seen "
                        f"embedding rows / {len(pattern_counts[ipattern])} "
                        f"original states"
                    )
        
                #
                # Zero unseen material embedding rows in the exported copy only.
                #
                if name.startswith("material_embedding"):
                    imaterial = int(name[len("material_embedding"):])
                    unseen = material_counts[imaterial] == 0
        
                    if weights.shape[0] != len(unseen):
                        raise ValueError(
                            f"{name}: embedding has {weights.shape[0]} rows, "
                            f"but material_counts has {len(unseen)} rows"
                        )
        
                    print(
                        f"material {imaterial}: zeroing "
                        f"{np.count_nonzero(unseen)} / {len(unseen)} "
                        f"({100.0 * np.mean(unseen):.2f}%) unseen embeddings"
                    )
        
                    weights[unseen, :] = 0.0
        
                weights_name = f"{name}_weights.csv"
                bias_name = f"{name}_bias.csv"
        
                files.append(
                    array_to_json_file(
                        weights_name,
                        weights,
                        "base64-float32",
                    )
                )
        
                files.append(
                    array_to_json_file(
                        bias_name,
                        biases,
                        "base64-float32",
                    )
                )
        
            directory = {
                "pragma": "dummy",
                "name": os.path.basename(os.path.normpath(self.model_dir)),
                "network2material_score": 1170.0,
                "shape": "kingsrow2",
                "architecture": "add",
                "architecture_patterns": ARCHITECTURE_PATTERNS,
                "architecture_material": ARCHITECTURE_MATERIAL,
                "materials": [
                    {"name": "nwhite_man"},
                    {"name": "nblack_man"},
                    {"name": "nwhite_king"},
                    {"name": "nblack_king"},
                    {"name": "man_king"},
                    {"name": "white_mobility"},
                    {"name": "black_mobility"},
                    {"name": "white_immobility"},
                    {"name": "black_immobility"},
                ],
                "activation_inputs": ACTIVATION_INPUTS,
                "activation": ACTIVATION,
                "activation_last": ACTIVATION_LAST,
                "files": files,
            }
        
            output = {
                "directories": [
                    directory,
                ]
            }
        
            json_path = os.path.join(
                self.model_dir,
                f"network-epoch{epoch + 1}.json",
            )
        
            # Write to a temporary file first so an interrupted save cannot leave
            # a partially written network JSON file.
            temporary_json_path = json_path + ".tmp"
        
            with open(temporary_json_path, "w", encoding="utf-8") as json_file:
                json.dump(
                    output,
                    json_file,
                    indent=2,
                    ensure_ascii=False,
                )
                json_file.write("\n")
        
            os.replace(temporary_json_path, json_path)
        
            print(
                f"Network saved to {json_path}: "
                f"{len(files)} encoded arrays"
            )
        
        # Collect predictions on the validation set
        all_predictions, all_labels = [], []
        for batch_features, batch_labels in self.validation_dataset.take(
            NVALIDATION_STEPS
        ):
            predictions = self.model.predict_on_batch(batch_features)
            labels = batch_labels.numpy().astype(np.float32)

            if ACTIVATION_LAST == "logits":
                labels = labels - 1.0

                logits = predictions - np.max(predictions, axis=1, keepdims=True)
                probabilities = np.exp(logits)
                probabilities /= np.sum(probabilities, axis=1, keepdims=True)

                predictions = probabilities[:, 2] - probabilities[:, 0]

            all_predictions.append(predictions.flatten())
            all_labels.append(labels.flatten())

        # Compute metrics on CPU
        with tf.device("CPU"):
            all_predictions = np.concatenate(all_predictions, axis=0)
            all_labels = np.concatenate(all_labels, axis=0)

            abs_differences = tf.abs(all_labels - all_predictions)
            epsilon = 1e-8
            rel_differences = abs_differences / (tf.abs(all_labels) + epsilon)

            percentiles = [90, 91, 92, 93, 94, 95, 96, 97, 98, 99]

            abs_percentiles = tfp.stats.percentile(
                abs_differences,
                q=percentiles,
                interpolation="nearest",
            )
            sorted_indices_abs = tf.argsort(abs_differences, axis=0)
            sorted_labels_abs = tf.gather(all_labels, sorted_indices_abs)
            abs_labels = tf.gather(
                sorted_labels_abs,
                [int(p * len(abs_differences) / 100) for p in percentiles],
            )

            rel_percentiles = tfp.stats.percentile(
                rel_differences,
                q=percentiles,
                interpolation="nearest",
            )
            sorted_indices_rel = tf.argsort(rel_differences, axis=0)
            sorted_labels_rel = tf.gather(all_labels, sorted_indices_rel)
            rel_labels = tf.gather(
                sorted_labels_rel,
                [int(p * len(rel_differences) / 100) for p in percentiles],
            )

        # Console report
        print(model_id)
        header = (
            f"{'Percentile':<15} {'Abs Error':<20} "
            f"{'Abs Error Label':<20} {'Rel Error':<20} "
            f"{'Rel Error Label':<20}"
        )
        print(f"Epoch {epoch + 1}")
        print(header)
        print("-" * 95)

        for i, p in enumerate(percentiles):
            print(
                f"{p:<15} {abs_percentiles[i]:<20.8f} {abs_labels[i]:<20.8f} "
                f"{rel_percentiles[i]:<20.8f} {rel_labels[i]:<20.8f}"
            )

        #print_wdl_scalar_diagnostics(all_predictions, all_labels)

        # Append to percentiles.txt
        with open(
            f"{self.model_dir}/percentiles.txt", "a", encoding="utf-8"
        ) as f:
            f.write(f"Epoch {epoch + 1}\n")
            f.write(header + "\n")
            f.write("-" * 95 + "\n")
            for i, p in enumerate(percentiles):
                f.write(
                    f"{p:<15} {abs_percentiles[i]:<20.8f} "
                    f"{abs_labels[i]:<20.8f} {rel_percentiles[i]:<20.8f} "
                    f"{rel_labels[i]:<20.8f}\n"
                )
            f.write("\n")

        # Prediction stats (console + file)
        mean_pred = np.mean(all_predictions)
        std_pred = np.std(all_predictions)
        min_pred = np.min(all_predictions)
        max_pred = np.max(all_predictions)

        print(
            f"Prediction stats: mean={mean_pred:.4f}, std={std_pred:.4f}, "
            f"min={min_pred:.4f}, max={max_pred:.4f}"
        )

        with open(
            f"{self.model_dir}/stats.txt", "a", encoding="utf-8"
        ) as stats_file:
            stats_file.write(f"Epoch {epoch + 1}\n\n")
            stats_file.write(
                f"mean={mean_pred:.6f}, std={std_pred:.6f}, "
                f"min={min_pred:.6f}, max={max_pred:.6f}\n"
            )

        # Save histogram PNG
        plt.figure(figsize=(10, 5))
        plt.hist(all_predictions, bins=100, alpha=0.7, edgecolor="black")
        plt.title(f"Prediction Distribution (Epoch {epoch + 1})")
        plt.xlabel("Prediction")
        plt.ylabel("Frequency")
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(f"{self.model_dir}/prediction{epoch + 1}.png")
        plt.close()

        # Keep this if you had it before (note: clearing sessions during training
        # can be risky)
        # backend.clear_session()


class LearningRateStoppingCallback(tf.keras.callbacks.Callback):
    def __init__(self, threshold=1.0e-6):
        super().__init__()
        self.threshold = threshold

    def on_epoch_end(self, epoch, logs=None):
        del logs  # unused
        lr = float(backend.get_value(self.model.optimizer.learning_rate))
        print(f"\nCurrent learning rate: {lr:.1e}")
        if lr < self.threshold:
            print(
                f"\nStopping training as learning rate dropped below "
                f"{self.threshold:.1e}."
            )
            self.model.stop_training = True


def create_checkpoint_callback(checkpoint_dir: str):
    return tf.keras.callbacks.ModelCheckpoint(
        filepath=f"{checkpoint_dir}/ckpt.keras",
        save_weights_only=False,
        monitor="val_loss",
        mode="min",
        save_best_only=True,
    )


# ----------------------------
# Data
# ----------------------------
(
    pattern_embed_sizes,
    compact_pattern_embed_sizes,
    pattern_state_maps,
    material_embed_sizes,
    training_data,
    validation_data,
    validation_seen_mask,
    pattern_counts,
    material_counts,
) = prepare_datasets()

NTRAINING_RECORDS = count_records(training_data)
NVALIDATION_RECORDS_TOTAL = count_records(validation_data)
NVALIDATION_RECORDS = int(np.count_nonzero(validation_seen_mask))
NTRAINING_STEPS = NTRAINING_RECORDS // BATCH_SIZE
NVALIDATION_STEPS = NVALIDATION_RECORDS // BATCH_SIZE

print(
    f"NTRAINING_RECORDS={NTRAINING_RECORDS} "
    f"NTRAINING_STEPS={NTRAINING_STEPS}"
)
print(
    f"NVALIDATION_RECORDS={NVALIDATION_RECORDS} "
    f"(from {NVALIDATION_RECORDS_TOTAL}) "
    f"NVALIDATION_STEPS={NVALIDATION_STEPS}"
)

PREVIEW_BATCH_SIZE = 40
print("Starting preview_dataset.take(1)")

preview_dataset = tf.data.Dataset.from_generator(
    lambda: raw_binary_batch_generator(
        validation_data,
        PREVIEW_BATCH_SIZE,
        pattern_state_maps,
        validation_seen_mask,
    ),
    output_signature=(
        (
            tf.TensorSpec(
                shape=(PREVIEW_BATCH_SIZE, NPATTERNS), dtype=tf.uint32
            ),
            tf.TensorSpec(
                shape=(PREVIEW_BATCH_SIZE, NMATERIALS), dtype=tf.uint32
            ),
        ),
        tf.TensorSpec(shape=(PREVIEW_BATCH_SIZE,), dtype=LABEL_TF_DTYPE),
    ),
)

# Print the first preview batch
for (dataset_patterns, dataset_material), dataset_labels in preview_dataset.take(1):
    print(dataset_patterns.shape)
    print(dataset_material.shape)
    print(dataset_labels.shape)
    np.set_printoptions(threshold=np.inf)
    print("validation patterns")
    print(dataset_patterns.numpy())
    print("validation material")
    print(dataset_material.numpy())
    print("validation labels")
    print(dataset_labels.numpy())

def make_batched_infinite_dataset(data, batch_size, record_mask=None):
    dataset = tf.data.Dataset.from_generator(
        lambda: raw_binary_batch_generator(
            data, batch_size, pattern_state_maps, record_mask
        ),
        output_signature=(
            (
                tf.TensorSpec((batch_size, NPATTERNS), dtype=tf.uint32),
                tf.TensorSpec((batch_size, NMATERIALS), dtype=tf.uint32),
            ),
            tf.TensorSpec((batch_size,), dtype=LABEL_TF_DTYPE),
        ),
    )
    return dataset.repeat()


training_dataset = make_batched_infinite_dataset(training_data, BATCH_SIZE)
validation_dataset = make_batched_infinite_dataset(
    validation_data, BATCH_SIZE, validation_seen_mask
)

# ----------------------------
# Model
# ----------------------------
input_patterns = tf.keras.Input(
    shape=(NPATTERNS,),
    dtype=tf.int32,
    name="pattern_indices",
)
input_material = tf.keras.Input(
    shape=(NMATERIALS,),
    dtype=tf.int32,
    name="material_index",
)

# Embedding for each pattern and material
pattern_embeddings = []
for i in range(NPATTERNS):
    embedding = layers.Embedding(
        input_dim=int(compact_pattern_embed_sizes[i]),
        output_dim=NEMBED_PATTERN,
        name=f"pattern_embedding{i}",
    )(input_patterns[:, i])
    pattern_embeddings.append(embedding)

material_embeddings = []
for i in range(NMATERIALS):
    l2 = 1e-5 if i < 4 else 1e-4
    if ARCHITECTURE_MATERIAL == "add":
        embedding = layers.Embedding(
            input_dim=material_embed_sizes[i],
            output_dim=NEMBED_MATERIAL,
            name=f"material_embedding{i}",
        )(input_material[:, i])
    else:
        embedding = layers.Embedding(
            input_dim=material_embed_sizes[i],
            output_dim=NEMBED_MATERIAL,
            name=f"material_embedding{i}",
            embeddings_initializer=tf.keras.initializers.Constant(
                np.eye(
                    material_embed_sizes[i],
                    NEMBED_MATERIAL,
                    dtype=np.float32,
                )
            ),
            trainable=False
        )(input_material[:, i])

    material_embeddings.append(embedding)

def orthogonal_kernel_regularizer(exponent=3):
    def regularizer(weights):
        scale = 10 ** (-exponent)
        shape = tf.shape(weights)
        rows = shape[0]  # input_dim
        cols = shape[1]  # output_dim

        def row_orthogonal():
            # W W^T ≈ I_rows  (good for wide or square)
            ww_t = tf.matmul(weights, weights, transpose_b=True)
            dim = tf.shape(ww_t)[0]
            identity = tf.eye(dim, dtype=weights.dtype)
            return scale * tf.reduce_sum(tf.square(ww_t - identity))

        def column_orthogonal():
            # W^T W ≈ I_cols  (good for tall)
            wtw = tf.matmul(weights, weights, transpose_a=True)
            dim = tf.shape(wtw)[0]
            identity = tf.eye(dim, dtype=weights.dtype)
            return scale * tf.reduce_sum(tf.square(wtw - identity))

        return tf.cond(rows <= cols, row_orthogonal, column_orthogonal)

    return regularizer

if ARCHITECTURE_PATTERNS == "add":
    pattern_inputs = layers.Add(name="pattern_sum")(pattern_embeddings)
    pattern_inputs = return_activation(ACTIVATION_INPUTS, pattern_inputs)
elif ARCHITECTURE_PATTERNS == "concat":
    pattern_inputs = layers.Concatenate()(pattern_embeddings)
elif ARCHITECTURE_PATTERNS == "null":
    pattern_inputs = None
else:
    raise ValueError(f"Unsupported ARCHITECTURE_PATTERNS: {ARCHITECTURE_PATTERNS}")

if ARCHITECTURE_MATERIAL == "add":
    material_inputs = layers.Add(name="material_sum")(material_embeddings)
    material_inputs = return_activation(ACTIVATION_INPUTS, material_inputs)
elif ARCHITECTURE_MATERIAL == "concat":
    material_inputs = layers.Concatenate()(material_embeddings)
elif ARCHITECTURE_MATERIAL == "null":
    material_inputs = None
else:
    raise ValueError(f"Unsupported ARCHITECTURE_MATERIAL: {ARCHITECTURE_MATERIAL}")

if pattern_inputs is None:
    pattern_hidden = None
else:
    pattern_hidden = layers.Dense(NLAYER0_PATTERN, use_bias=USE_BIAS, name="pattern_dense0")(pattern_inputs)
    pattern_hidden = return_activation(ACTIVATION, pattern_hidden)

    if NLAYER1_PATTERN > 0:
        pattern_hidden = layers.Dense(NLAYER1_PATTERN, use_bias=USE_BIAS, name="pattern_dense1")(pattern_hidden)
        pattern_hidden = return_activation(ACTIVATION, pattern_hidden)
    
    if NLAYER2_PATTERN > 0:
        pattern_hidden = layers.Dense(NLAYER2_PATTERN, use_bias=USE_BIAS, name="pattern_dense2")(pattern_hidden)
        pattern_hidden = return_activation(ACTIVATION, pattern_hidden)

if material_inputs is None:
    material_hidden = None
else:
    material_hidden = layers.Dense(NLAYER0_MATERIAL, use_bias=USE_BIAS, name="material_dense0")(material_inputs)
    material_hidden = return_activation(ACTIVATION, material_hidden)

    if NLAYER1_MATERIAL > 0:
        material_hidden = layers.Dense(NLAYER1_MATERIAL, use_bias=USE_BIAS, name="material_dense1")(material_hidden)
        material_hidden = return_activation(ACTIVATION, material_hidden)
    
    if NLAYER2_MATERIAL > 0:
        material_hidden = layers.Dense(NLAYER2_MATERIAL, use_bias=USE_BIAS, name="material_dense2")(material_hidden)
        material_hidden = return_activation(ACTIVATION, material_hidden)


inputs = layers.Concatenate()([pattern_hidden, material_hidden])

hidden = layers.Dense(
    NLAYER0,
    use_bias=USE_BIAS,
    name="final_dense0"
)(inputs)

hidden = return_activation(ACTIVATION, hidden)

if NLAYER1 > 0:
    hidden = layers.Dense(
        NLAYER1,
        use_bias=USE_BIAS,
        name="final_dense1"
    )(hidden)
    hidden = return_activation(ACTIVATION, hidden)

if NLAYER2 > 0:
    hidden = layers.Dense(
        NLAYER2,
        use_bias=USE_BIAS,
        name="final_dense2"
    )(hidden)
    hidden = return_activation(ACTIVATION, hidden)

if ACTIVATION_LAST == "logits":
    output = layers.Dense(
        3,
        activation=None,
        use_bias=USE_BIAS,
        name="final_dense3",
    )(hidden)
else:
    output = layers.Dense(
        1,
        activation=ACTIVATION_LAST,
        use_bias=USE_BIAS,
        name="final_dense3",
    )(hidden)

gwd_model = models.Model(inputs=[input_patterns, input_material], outputs=output)

layer_units = {
    "p": [],
    "m": [],
    "f": [],
}

for layer in gwd_model.layers:
    if not hasattr(layer, "units"):
        continue

    name = layer.name

    if name.startswith("pattern_dense"):
        layer_units["p"].append(str(layer.units))
    elif name.startswith("material_dense"):
        layer_units["m"].append(str(layer.units))
    elif name.startswith("final_dense"):
        layer_units["f"].append(str(layer.units))

pattern_units = (
    "x".join(layer_units["p"])
)
material_units = (
    "x".join(layer_units["m"])
)
final_units = (
    "x".join(layer_units["f"])
)

model_id = (
    f"np+{NPATTERNS}"
    f"-nm+{NMATERIALS}"
    f"-p{ARCHITECTURE_PATTERNS}x{NEMBED_PATTERN}x{pattern_units}"
    f"-m{ARCHITECTURE_MATERIAL}x{NEMBED_MATERIAL}x{material_units}"
    f"-f{final_units}"
    f"-e+{EXPONENT}"
    f"-ai+{ACTIVATION_INPUTS}"
    f"-a+{ACTIVATION}"
    f"-al+{ACTIVATION_LAST}"
    f"-b+{int(USE_BIAS)}"
    f"-l+{LOSS_NAME}"
    f"-{DATA}"
    f"{'-' + TAG if TAG else ''}"
)

model_dir = f"{ROOT}/tf/networks/{model_id}"
os.makedirs(model_dir, exist_ok=True)

# Write options.json fragment
options_path = os.path.join(model_dir, "options.json")
options = (
    f'      "network2material_score": 1170.0,\n'
    f'      "shape": "kingsrow2",\n'
    f'      "architecture_patterns": "{ARCHITECTURE_PATTERNS}",\n'
    f'      "architecture_material": "{ARCHITECTURE_MATERIAL}",\n'
    f'      "materials": [\n'
    f'         {{"name": "nwhite_man"}},\n'
    f'         {{"name": "nblack_man"}},\n'
    f'         {{"name": "nwhite_king"}},\n'
    f'         {{"name": "nblack_king"}},\n'
    f'         {{"name": "man_king"}},\n'
    f'         {{"name": "white_mobility"}},\n'
    f'         {{"name": "black_mobility"}},\n'
    f'         {{"name": "white_immobility"}},\n'
    f'         {{"name": "black_immobility"}}\n'
    f'      ],\n'
    f'      "activation_inputs": "{ACTIVATION_INPUTS}",\n'
    f'      "activation": "{ACTIVATION}",\n'
    f'      "activation_last": "{ACTIVATION_LAST}",\n'
)
with open(options_path, "w", encoding="utf-8") as options_file:
    options_file.write(options)
print(f"Options saved to {options_path}")

checkpoint_dir = f"{ROOT}/tf/checkpoints/{model_id}"
os.makedirs(checkpoint_dir, exist_ok=True)

print(model_id)
print(checkpoint_dir)
print(model_dir)

checkpoint_callback = create_checkpoint_callback(checkpoint_dir)

reduce_lr = tf.keras.callbacks.ReduceLROnPlateau(
    monitor="val_loss", factor=0.96, patience=PATIENCE
)
early_stopping = tf.keras.callbacks.EarlyStopping(
    monitor="val_loss",
    patience=1000,
)
lr_stopping_callback = LearningRateStoppingCallback()

epoch_end_callback = EpochEnd(
    validation_dataset, model_dir, interval=INTERVAL
)

huber = tf.keras.losses.Huber(delta=0.25)
mse = tf.keras.losses.MeanSquaredError()
sccentropy = tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True)

def huber_loss32(y_true, y_pred):
    y_pred32 = tf.cast(y_pred, tf.float32)
    y_true32 = tf.cast(y_true, tf.float32)
    return huber(y_true32, y_pred32)

# ----------------------------
# Compile
# ----------------------------

if LOSS_NAME == "huber":
    gwd_model.compile(
        optimizer=tf.keras.optimizers.AdamW(),
        loss=huber,
    )
elif LOSS_NAME == "mse":
    gwd_model.compile(
        optimizer=tf.keras.optimizers.AdamW(),
        loss=mse,
        metrics=[tf.keras.metrics.MeanAbsoluteError(name="mae")]
    )
elif LOSS_NAME == "sccentropy":
    gwd_model.compile(
        optimizer=tf.keras.optimizers.AdamW(),
        loss=sccentropy,
        metrics=[tf.keras.metrics.SparseCategoricalAccuracy(name="acc")],
    )
else:
    raise ValueError(f"Unsupported loss function: {LOSS_NAME}")

gwd_model.summary()

# ----------------------------
# Train
# ----------------------------
history = gwd_model.fit(
    training_dataset,
    epochs=NEPOCHS,
    steps_per_epoch=NTRAINING_STEPS,
    validation_data=validation_dataset,
    validation_steps=NVALIDATION_STEPS,
    callbacks=[reduce_lr, early_stopping, lr_stopping_callback, epoch_end_callback]
)

