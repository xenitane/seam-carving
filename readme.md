# seam-carving

> An implementation of the seam carving algorithm to rescale images.
>
> This implementation uses the stb headers for image I/O

## Setup

> Have a `C` compiler installed on your system. I'm using `gcc` here, but if you have another one, change the contents of the `cc` function in `nob.c`\'s first line for your compiler.

```bash
    cc -o nob nob.c
```

This will generate the nob binary, run it to get started.

## Example Images

<table>
    <thead>
        <tr>
            <th>Original Image</th>
            <th>After removing 50px from width</th>
            <th>After removing 100px from width</th>
            <th>After removing 200px from width</th>
        </tr>
    </thead>
    <tr>
        <td max-width="25%" height="120px"><img src="images/test_0.jpg"></td>
        <td max-width="25%" height="120px"><img src="images/output_0_300.png"></td>
        <td max-width="25%" height="120px"><img src="images/output_0_400.png"></td>
        <td max-width="25%" height="120px"><img src="images/output_0_500.png"></td>
    <tr>
    <tr>
        <td max-width="25%" height="120px"><img src="images/test_1.jpg"></td>
        <td max-width="25%" height="120px"><img src="images/output_1_300.png"></td>
        <td max-width="25%" height="120px"><img src="images/output_1_400.png"></td>
        <td max-width="25%" height="120px"><img src="images/output_1_500.png"></td>
    <tr>
    <tr>
        <td max-width="20%" height="120px"><img src="images/test_2.jpg"></td>
        <td max-width="20%" height="120px"><img src="images/output_2_300.png"></td>
        <td max-width="20%" height="120px"><img src="images/output_2_400.png"></td>
        <td max-width="20%" height="120px"><img src="images/output_2_500.png"></td>
    <tr>
    <tbody>
    </tbody>
</table>

## References

- <https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color>
- <https://en.wikipedia.org/wiki/Seam_carving>
- <https://dl.acm.org/doi/10.1145/1276377.1276390>
- <https://github.com/nothings/stb>
