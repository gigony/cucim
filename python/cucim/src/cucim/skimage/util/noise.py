import cupy as cp

from .dtype import img_as_float

__all__ = ['random_noise']


def random_noise(image, mode='gaussian', seed=None, clip=True, **kwargs):
    """
    Function to add random noise of various types to a floating-point image.

    Parameters
    ----------
    image : ndarray
        Input image data. Will be converted to float.
    mode : str, optional
        One of the following strings, selecting the type of noise to add:

        'gaussian' (default)
            Gaussian-distributed additive noise.
        'localvar'
            Gaussian-distributed additive noise, with specified local variance
            at each point of `image`.
        'poisson'
            Poisson-distributed noise generated from the data.
        'salt'
            Replaces random pixels with 1.
        'pepper'
            Replaces random pixels with 0 (for unsigned images) or -1 (for
            signed images).
        's&p'
            Replaces random pixels with either 1 or `low_val`, where `low_val`
            is 0 for unsigned images or -1 for signed images.
        'speckle'
            Multiplicative noise using ``out = image + n * image``, where ``n``
            is Gaussian noise with specified mean & variance.
    seed : int, optional
        If provided, this will set the random seed before generating noise,
        for valid pseudo-random comparisons.
    clip : bool, optional
        If True (default), the output will be clipped after noise applied
        for modes `'speckle'`, `'poisson'`, and `'gaussian'`. This is
        needed to maintain the proper image data range. If False, clipping
        is not applied, and the output may extend beyond the range [-1, 1].
    mean : float, optional
        Mean of random distribution. Used in 'gaussian' and 'speckle'.
        Default : 0.
    var : float, optional
        Variance of random distribution. Used in 'gaussian' and 'speckle'.
        Note: variance = (standard deviation) ** 2. Default : 0.01
    local_vars : ndarray, optional
        Array of positive floats, same shape as `image`, defining the local
        variance at every image point. Used in 'localvar'.
    amount : float, optional
        Proportion of image pixels to replace with noise on range [0, 1].
        Used in 'salt', 'pepper', and 'salt & pepper'. Default : 0.05
    salt_vs_pepper : float, optional
        Proportion of salt vs. pepper noise for 's&p' on range [0, 1].
        Higher values represent more salt. Default : 0.5 (equal amounts)

    Returns
    -------
    out : ndarray
        Output floating-point image data on range [0, 1] or [-1, 1] if the
        input `image` was unsigned or signed, respectively.

    Notes
    -----
    Speckle, Poisson, Localvar, and Gaussian noise may generate noise outside
    the valid image range. The default is to clip (not alias) these values,
    but they may be preserved by setting `clip=False`. Note that in this case
    the output may contain values outside the ranges [0, 1] or [-1, 1].
    Use this option with care.

    Because of the prevalence of exclusively positive floating-point images in
    intermediate calculations, it is not possible to intuit if an input is
    signed based on dtype alone. Instead, negative values are explicitly
    searched for. Only if found does this function assume signed input.
    Unexpected results only occur in rare, poorly exposes cases (e.g. if all
    values are above 50 percent gray in a signed `image`). In this event,
    manually scaling the input to the positive domain will solve the problem.

    The Poisson distribution is only defined for positive integers. To apply
    this noise type, the number of unique values in the image is found and
    the next round power of two is used to scale up the floating-point result,
    after which it is scaled back down to the floating-point image range.

    To generate Poisson noise against a signed image, the signed image is
    temporarily converted to an unsigned image in the floating point domain,
    Poisson noise is generated, then it is returned to the original range.

    """
    mode = mode.lower()

    # Detect if a signed image was input
    if image.min() < 0:
        low_clip = -1.0
    else:
        low_clip = 0.0

    image = img_as_float(image)
    if seed is not None:
        cp.random.seed(seed=seed)

    allowedtypes = {
        'gaussian': 'gaussian_values',
        'localvar': 'localvar_values',
        'poisson': 'poisson_values',
        'salt': 'sp_values',
        'pepper': 'sp_values',
        's&p': 's&p_values',
        'speckle': 'gaussian_values'}

    kwdefaults = {
        'mean': 0.0,
        'var': 0.01,
        'amount': 0.05,
        'salt_vs_pepper': 0.5,
        'local_vars': cp.zeros_like(image) + 0.01}

    allowedkwargs = {
        'gaussian_values': ['mean', 'var'],
        'localvar_values': ['local_vars'],
        'sp_values': ['amount'],
        's&p_values': ['amount', 'salt_vs_pepper'],
        'poisson_values': []}

    for key in kwargs:
        if key not in allowedkwargs[allowedtypes[mode]]:
            raise ValueError(
                f"{key} keyword not in allowed keywords "
                f"{allowedkwargs[allowedtypes[mode]]}"
            )
    # Set kwarg defaults
    for kw in allowedkwargs[allowedtypes[mode]]:
        kwargs.setdefault(kw, kwdefaults[kw])

    if mode == 'gaussian':
        noise = cp.random.normal(kwargs['mean'], kwargs['var'] ** 0.5,
                                 image.shape)
        out = image + noise

    elif mode == 'localvar':
        # Ensure local variance input is correct
        if (kwargs['local_vars'] <= 0).any():
            raise ValueError('All values of `local_vars` must be > 0.')

        # Safe shortcut usage broadcasts kwargs['local_vars'] as a ufunc

        # CuPy Backend: Must supply size argument to get around a CuPy bug
        #       https://github.com/cupy/cupy/pull/4457
        out = image + cp.random.normal(
            0, kwargs["local_vars"] ** 0.5, kwargs["local_vars"].shape
        )

    elif mode == 'poisson':
        # Determine unique values in image & calculate the next power of two
        vals = len(cp.unique(image))
        vals = 2 ** cp.ceil(cp.log2(vals))

        # Ensure image is exclusively positive
        if low_clip == -1.0:
            old_max = image.max()
            image = (image + 1.0) / (old_max + 1.0)

        # Generating noise for each unique value in image.
        out = cp.random.poisson(image * vals) / float(vals)

        # Return image to original range if input was signed
        if low_clip == -1.0:
            out = out * (old_max + 1.0) - 1.0

    elif mode == 'salt':
        # Re-call function with mode='s&p' and p=1 (all salt noise)
        out = random_noise(image, mode='s&p', seed=seed,
                           amount=kwargs['amount'], salt_vs_pepper=1.)

    elif mode == 'pepper':
        # Re-call function with mode='s&p' and p=1 (all pepper noise)
        out = random_noise(image, mode='s&p', seed=seed,
                           amount=kwargs['amount'], salt_vs_pepper=0.)

    elif mode == 's&p':
        out = image.copy()
        p = kwargs['amount']
        q = kwargs['salt_vs_pepper']
        flipped = cp.random.choice([True, False], size=image.shape,
                                   p=[p, 1 - p])
        salted = cp.random.choice([True, False], size=image.shape,
                                  p=[q, 1 - q])
        peppered = ~salted
        out[flipped & salted] = 1
        out[flipped & peppered] = low_clip

    elif mode == 'speckle':
        noise = cp.random.normal(kwargs['mean'], kwargs['var'] ** 0.5,
                                 image.shape)
        out = image + image * noise

    # Clip back to original range, if necessary
    if clip:
        out = cp.clip(out, low_clip, 1.0)

    return out
