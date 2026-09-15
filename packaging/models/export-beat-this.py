import argparse
import pathlib
import struct
import sys

import numpy
import torch

MAGIC = b"HTMBEAT\0"
FORMAT_VERSION = 1
FRONTEND_BLOCKS = 3
TRANSFORMER_LAYERS = 6
PARTIAL_DIRECTIONS = ("F", "T")


def matrix(state: dict[str, torch.Tensor], name: str) -> numpy.ndarray:
    """Return a linear weight as a row-major input by output matrix."""
    return state[name].detach().numpy().astype(numpy.float32).T.copy()


def vector(state: dict[str, torch.Tensor], name: str) -> numpy.ndarray:
    """Return a one dimensional parameter as it is stored."""
    return state[name].detach().numpy().astype(numpy.float32).copy()


def kernel(state: dict[str, torch.Tensor], name: str) -> numpy.ndarray:
    """Return a convolution weight flattened to output channels by input taps."""
    weight = state[name].detach().numpy().astype(numpy.float32)
    return weight.reshape(weight.shape[0], -1).copy()


def batch_norm(state: dict[str, torch.Tensor], source: str, target: str) -> dict[str, numpy.ndarray]:
    """Return the four evaluation parameters of a batch normalization layer."""
    return {
        target + ".weight": vector(state, source + ".weight"),
        target + ".bias": vector(state, source + ".bias"),
        target + ".mean": vector(state, source + ".running_mean"),
        target + ".variance": vector(state, source + ".running_var"),
    }


def attention(state: dict[str, torch.Tensor], source: str, target: str) -> dict[str, numpy.ndarray]:
    """Return the gated rotary attention parameters of one block."""
    return {
        target + ".rotary.frequencies": vector(state, source + ".rotary_embed.freqs"),
        target + ".norm.gamma": vector(state, source + ".norm.gamma"),
        target + ".qkv.weight": matrix(state, source + ".to_qkv.weight"),
        target + ".gates.weight": matrix(state, source + ".to_gates.weight"),
        target + ".gates.bias": vector(state, source + ".to_gates.bias"),
        target + ".out.weight": matrix(state, source + ".to_out.0.weight"),
    }


def feed_forward(state: dict[str, torch.Tensor], source: str, target: str) -> dict[str, numpy.ndarray]:
    """Return the normalized two layer feed forward parameters of one block."""
    return {
        target + ".norm.gamma": vector(state, source + ".net.0.gamma"),
        target + ".in.weight": matrix(state, source + ".net.1.weight"),
        target + ".in.bias": vector(state, source + ".net.1.bias"),
        target + ".out.weight": matrix(state, source + ".net.4.weight"),
        target + ".out.bias": vector(state, source + ".net.4.bias"),
    }


def collect(state: dict[str, torch.Tensor]) -> dict[str, numpy.ndarray]:
    """Return every tensor the C++ port reads, keyed by its name in the blob."""
    tensors: dict[str, numpy.ndarray] = {}
    tensors.update(batch_norm(state, "frontend.stem.bn1d", "stem.input"))
    tensors["stem.conv.weight"] = kernel(state, "frontend.stem.conv2d.weight")
    tensors.update(batch_norm(state, "frontend.stem.bn2d", "stem.norm"))
    for index in range(FRONTEND_BLOCKS):
        source = "frontend.blocks." + str(index)
        target = "frontend." + str(index)
        for direction in PARTIAL_DIRECTIONS:
            tensors.update(
                attention(state, source + ".partial.attn" + direction, target + ".attention" + direction)
            )
            tensors.update(
                feed_forward(state, source + ".partial.ff" + direction, target + ".forward" + direction)
            )
        tensors[target + ".conv.weight"] = kernel(state, source + ".conv2d.weight")
        tensors.update(batch_norm(state, source + ".norm", target + ".norm"))
    tensors["frontend.linear.weight"] = matrix(state, "frontend.linear.weight")
    tensors["frontend.linear.bias"] = vector(state, "frontend.linear.bias")
    for index in range(TRANSFORMER_LAYERS):
        source = "transformer_blocks.layers." + str(index)
        target = "transformer." + str(index)
        tensors.update(attention(state, source + ".0", target + ".attention"))
        tensors.update(feed_forward(state, source + ".1", target + ".forward"))
    tensors["transformer.norm.gamma"] = vector(state, "transformer_blocks.norm.gamma")
    tensors["head.weight"] = matrix(state, "task_heads.beat_downbeat_lin.weight")
    tensors["head.bias"] = vector(state, "task_heads.beat_downbeat_lin.bias")
    return tensors


def model_state(checkpoint: pathlib.Path) -> dict[str, torch.Tensor]:
    """Return the model tensors of a Beat This lightning checkpoint."""
    loaded = torch.load(checkpoint, map_location="cpu", weights_only=True)
    prefix = "model."
    return {
        name[len(prefix) :]: value
        for name, value in loaded["state_dict"].items()
        if name.startswith(prefix) and value.dtype == torch.float32
    }


def encode(tensors: dict[str, numpy.ndarray]) -> bytes:
    """Return the blob holding the tensor index and every value as little endian float32."""
    index = bytearray(MAGIC)
    index += struct.pack("<II", FORMAT_VERSION, len(tensors))
    body = bytearray()
    for name, values in tensors.items():
        encoded = name.encode("ascii")
        index += struct.pack("<H", len(encoded)) + encoded
        index += struct.pack("<B", values.ndim)
        for extent in values.shape:
            index += struct.pack("<I", extent)
        body += values.astype("<f4").tobytes()
    return bytes(index + body)


def main() -> int:
    """Convert a Beat This checkpoint into the blob the application embeds."""
    parser = argparse.ArgumentParser(description="Export Beat This weights for ht-music.")
    parser.add_argument("--checkpoint", required=True, help="Path to the beat_this-small0.ckpt file.")
    parser.add_argument("--output", required=True, help="Path of the blob to write.")
    arguments = parser.parse_args()

    state = model_state(pathlib.Path(arguments.checkpoint))
    tensors = collect(state)
    blob = encode(tensors)
    output = pathlib.Path(arguments.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(blob)
    values = sum(int(tensor.size) for tensor in tensors.values())
    print(f"wrote {output} with {len(tensors)} tensors, {values} values, {len(blob)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
