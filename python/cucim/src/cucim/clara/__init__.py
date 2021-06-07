#
# Copyright (c) 2020-2021, NVIDIA CORPORATION.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os

from . import cli, converter
# import hidden methods
from ._cucim import __version__, filesystem, io, cache

__all__ = ['cli', 'CuImage', 'filesystem', 'io', 'cache', 'converter', '__version__']


from ._cucim import _get_plugin_root  # isort:skip
from ._cucim import _set_plugin_root  # isort:skip
# Set plugin root path
_set_plugin_root(os.path.dirname(os.path.realpath(__file__)))


# update
class CuImage:
    def __init__(self, *args, **kwargs):
        self._C = _cucim.CuImage(*args, **kwargs)

    def __getattr__(self, attr):
        return getattr(self._C, attr)

    def __setattr__(self, attr, val):
        if attr == '_C':
            object.__setattr__(self, attr, val)
        else:
            setattr(self._C, attr, val)

    def read_region(self, *args, **kwargs):
        region = self._C.read_region(*args, **kwargs)
        _cucim.CuImage._set_array_interface(region)
        return region

# inherit

# class CuImage(_cucim.CuImage):
#     def read_region(self, *args, **kwargs):
#         region = super().read_region(*args, **kwargs)
#         super()._set_array_interface(region)
#         return region