<p align="left"><img alt="OSPTEK" src="./images/logo.png" width="200" /></p>

<h1 align="center">OSPTEK 7.09″ EPD 1200×1600 (TBD · SPI)</h1>

<p align="center"><b>E-paper module · SPI · TBD</b></p>

<p align="center">English | <a href="./README.md">简体中文</a></p>

<p align="center">
  <img alt="Size: 7.09 inch" src="https://img.shields.io/badge/Size-7.09%22-3498DB?style=flat-square" />
  <img alt="Resolution: 1200x1600" src="https://img.shields.io/badge/Resolution-1200%C3%971600-8E44AD?style=flat-square" />
  <img alt="Interface: SPI" src="https://img.shields.io/badge/Interface-SPI-27AE60?style=flat-square" />
  <img alt="Driver: TBD" src="https://img.shields.io/badge/Driver-TBD-E7352C?style=flat-square" />
</p>

<p align="center"><img alt="OSPTEK 7.09&quot; 1200×1600 EPD SPI module (TBD) product image" src="./images/product.png" width="640" /></p>

## Contents

- [Overview](#overview)
- [Specifications](#specifications)
- [Sample projects](#sample-projects)
- [Repository layout](#repository-layout)
- [Resources](#resources)
- [Where to Buy](#where-to-buy)
- [Support](#support)

---

## Overview

OSPTEK **7.09″ 1200×1600 EPD** is a **SPI** six-color e-paper display module (black / white / red / yellow / blue / green). The driver IC part number is not confirmed yet, so the repository name uses **TBD** for now; it will be renamed and this page updated once confirmed.

Repo id: `epd-7.09-1200x1600-spi-tbd`

Current module version: **EPD0709A02**. Electrical and mechanical details follow [`docs/EPD0709A02.pdf`](./docs/EPD0709A02.pdf).

## Specifications

| Item | Spec |
| ---- | ---- |
| Size | 7.09 inch |
| Type | EPD (e-paper · six-color) |
| Resolution | 1200×1600 |
| Interface | SPI |
| Driver IC | TBD |

> Full outline, FPC definition, power, and timing follow the product datasheet / driver IC datasheet.

## Sample projects

| Description | Path |
| ---- | ---- |
| ESP32-S3 · six-color bar bring-up | [`examples/esp32s3-epd-7.09-1200x1600-spi-tbd-bringup/`](./examples/esp32s3-epd-7.09-1200x1600-spi-tbd-bringup/) |

## Repository layout

```text
epd-7.09-1200x1600-spi-tbd/                                # repo root (nav: ../../README_EN.md)
└── versions/
    └── EPD0709A02/                                # full materials for this part number
        ├── README.md
        ├── README_EN.md
        ├── images/
        ├── docs/
        └── examples/
```

## Resources

| Resource | Link |
| -------- | ---- |
| Product datasheet (EPD0709A02) | [`docs/EPD0709A02.pdf`](./docs/EPD0709A02.pdf) |
| 7.09″ six-color e-paper adapter schematic (Altium, V1.1) | [`docs/7.09寸6色墨水屏.SchDoc`](./docs/7.09%E5%AF%B86%E8%89%B2%E5%A2%A8%E6%B0%B4%E5%B1%8F.SchDoc) |

### Samples

- [ESP32-S3 six-color bar bring-up](./examples/esp32s3-epd-7.09-1200x1600-spi-tbd-bringup/)

## Where to Buy

<p align="center">
  <a href="https://www.aliexpress.com/store/1105701619"><img alt="AliExpress Official Store" src="https://img.shields.io/badge/AliExpress-Official_Store-E62E04?style=for-the-badge&logo=aliexpress&logoColor=white" /></a>
  &nbsp;&nbsp;
  <a href="https://shop110742373.taobao.com/"><img alt="Taobao Official Store" src="https://img.shields.io/badge/Taobao-Official_Store-FF6A00?style=for-the-badge" /></a>
</p>

**International (AliExpress)**

- Store: [OSPTEK Official Store](https://www.aliexpress.com/store/1105701619)

**China (Taobao)**

- Store: [鱼鹰光电工厂店](https://shop110742373.taobao.com/)

## Support

- Technical Support / Sales: <luyu@osptek.com>
- QQ Technical Group: **985881096**
- Website: <https://osptek.com/>
- Feel free to open an Issue in this repository if you have any questions

---

<p align="center"><sub>© 2026 OSPTEK · Licensed under CC BY 4.0</sub></p>
