from my_lib import Object

import os

from my_lib import Object3

from my_lib import Object2

import sys

from third_party import lib15, lib1, lib2, lib3, lib4, lib5, lib6, lib7, lib8, lib9, lib10, lib11, lib12, lib13, lib14

import sys

from __future__ import absolute_import

from third_party import lib3

print("Hey")
print("yo")

parserForComic = subparsers.add_parser("comic")
parserForComic.add_argument("wut_comic", choices=["xkcd", "ext"])
parserForComic.add_argument(
    "--save-as", type=str, default=None, help="save comic as"
)
parserForComic.add_argument(
    "--download-from", type=str, default=None, help="download import page"
)
parserForComic.add_argument("--pdf", action="store_true", help="create pdf")
