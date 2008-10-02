import rostools; rostools.update_path('test_roslaunch')

import sys
sys.path.append('lib')
import starfeature as L
import Image
import ImageChops
import ImageDraw

import random
import time
import unittest

# Small target in middle of image invartiant wrt image size
# A star detector object's results are invariant wrt image presentation order

def simple(im, scales = 7, threshold = 30.0, line_threshold = 10.0):
    sd = L.star_detector(im.size[0], im.size[1], scales, threshold, line_threshold)
    kp = sd.detect(im.tostring())
    return [ i[1] for i in sorted([ (abs(response), (x, y, s, response)) for (x, y, s, response) in kp])]

def circle(im, x, y, r, color):
    draw = ImageDraw.Draw(im)
    box = [ int(i) for i in [ x - r, y - r, x + r, y + r ]]
    draw.pieslice(box, 0, 360, fill = color)

class TestDirected(unittest.TestCase):

    def setUp(self):
        self.im640 = Image.open("im640x480.pgm")
        self.im64 = self.im640.crop((100,100,164,164))

    def test_a_golden(self):

        """ Verify that points match a fixed 'expected' list """

        golden = [
            (220, 308, 2, -108.11),
            (260, 83, 4, 108.23),
            (59, 303, 3, -108.35),
            (77, 337, 2, 108.62),
            (243, 173, 4, -108.66),
            (444, 397, 2, 108.75),
            (278, 315, 2, 108.92),
            (166, 375, 3, 109.03),
            (565, 62, 2, 110.78),
            (329, 374, 4, 110.84),
            (492, 298, 4, 111.09),
            (575, 277, 5, -112.04),
            (310, 254, 2, 112.50),
            (253, 303, 2, 114.18),
            (422, 364, 6, 114.35),
            (283, 251, 2, 114.42),
            (62, 373, 2, 114.71),
            (329, 186, 6, -117.83),
            (260, 112, 4, -118.80),
            (338, 398, 3, 119.23),
            (440, 283, 4, 119.94),
            (81, 334, 4, 120.82),
            (494, 347, 4, 121.11),
            (330, 182, 6, -121.18),
            (402, 350, 3, 123.50),
            (468, 283, 2, 124.68),
            (329, 192, 6, -126.28),
            (269, 216, 4, -132.43),
            (208, 225, 2, -145.93),
            (511, 92, 5, -149.24),
        ]

        result = simple(self.im640)[-30:]
        for a,e in zip(result, golden):
            self.assertEqual(a[0], e[0])
            self.assertEqual(a[1], e[1])
            self.assertEqual(a[2], e[2])
            self.assertAlmostEqual(a[3], e[3], 1)

    def test_blank(self):
        """ Blank images have 0 keypoints """
        
        for xsize in [640]:
            for ysize in [480]:
                for c in [0,128,255]:
                    self.assertEqual(simple(Image.new("L", (xsize,ysize), (c)), 7, 0.1), [])

    def test_r(self):
        """ Line of circles (varying r) across image """

        im = Image.new("L", (1000, 200), (0))
        npoints = 18
        xs = [ (100 + 50 * t) for t in range(npoints) ]
        for t in range(npoints):
            r = (2.0+0.5*t)
            circle(im, xs[t], 100, r, 248)

        # Add noise into the image.  If the image does not contain noise,
        # then the non maximum suppression can - like Buridan's ass - be
        # presented with two adjacent responses that are equal, and reject
        # both because neither is a maximum.  The chance of this happening
        # with real-world images is very remote indeed.

        noise = Image.fromstring("L", (1000,200), "".join([ chr(random.randrange(0, 8)) for i in range(1000 * 200)]))
        im = ImageChops.add(im, noise)

        result = sorted([(x,y,s,response) for (x,y,s,response) in simple(im, 7, 1.0, 999999.0)][-npoints:])

        # Must have npoints
        self.assertEqual(len(result), npoints)

        # X coordinates must be within 1 point of expected
        for i,(x,y,s,r) in enumerate(result):
            self.assert_(abs(x - xs[i]) <= 1)

        # Already ordered by x, so scale should be increasing
        ss = [s for (x,y,s,r) in result]
        self.assertEqual(ss, sorted(ss))

        if 0:
            print "GOT:", result
            for (x,y,s,r) in result:
                print (x,y,s,r)

    def test_L_symmetry(self):
        """
        image and negated image should have same keypoints
        """
        ref = simple(self.im640)
        self.assert_(len(ref) != 0) # if no hits in the image, have a test bug
        result = simple(ImageChops.invert(self.im640))
        self.assertEqual(len(ref), len(result))
        for (a,e) in zip(result, ref):
            self.assertEqual(a[0], e[0])
            self.assertEqual(a[1], e[1])
            self.assertEqual(a[2], e[2])
            self.assertEqual(a[3], -e[3])

    def test_translation(self):
        """ Small target translated in center of image has the same keypoints """

        def xlat(x, y):
            im = Image.new("L", (200, 200), (0))
            im.paste(self.im64, (x, y))
            return im

        ref = [(x-70,y-70,s,r) for (x,y,s,r) in simple(xlat(70, 70))]
        self.assert_(len(ref) != 0) # if no hits in the image, have a test bug
        for tx in [69,70,71,77]:
            for ty in [69,70,71,77]:
                result = simple(xlat(tx, ty))
                self.assertEqual(ref, [(x-tx,y-ty,s,r) for (x,y,s,r) in result])

    def test_subimage(self):
        """ Small target translated in center of image has the same keypoints """

        def make_big(x, y):
            im = Image.new("L", (x, y), (0))
            im.paste(self.im64, (100, 100))
            return im

        ref = simple(make_big(200, 200))
        self.assert_(len(ref) != 0) # if no hits in the image, have a test bug
        for sx in [200]:
            for sy in [200]:
                result = simple(make_big(sx, sy))
                self.assertEqual(ref, result)

    def test_xy_symmetry(self):
        """
        Flipped,flopped,rotated images have the same number of keypoints
        """

        for dim in [248]:
            refim = self.im640.resize((dim,dim))
            ref = simple(refim, 7, 30.0, 999999.0)
            self.assert_(len(ref) != 0) # if no hits in the image, have a test bug
            for f in [Image.FLIP_LEFT_RIGHT, Image.FLIP_TOP_BOTTOM, Image.ROTATE_90, Image.ROTATE_180, Image.ROTATE_270]:
                result = simple(refim.transpose(f), 7, 30.0, 999999.0)
                self.assertEqual(len(ref), len(result))

    def xtest_all_scales(self):
        """ random circles all over screen - pass if all scales get hit """

        im = Image.new("L", (1000, 1000), (0))
        random.seed(0)
        for t in range(100):
            circle(im, random.randrange(100, 900), random.randrange(100,900), 0.1 * t, 255)
        result = simple(im, 7)
        self.assertEqual(set([s for (x,y,s,r) in result]), set([2,3,4,5,6]))

    def test_L(self):
        """ Line of circles (varying L) across image """

        im = Image.new("L", (1000, 200), (0))
        for t in range(15):
            circle(im, 100 + 50 * t, 100, 3.5, t * 16)
        result = simple(im, 7, 1.0)

        # Should be 14 responses
        self.assertEqual(len(result), 14)

        # Results already ordered by response; check that X coordinates ascend
        xs = [i[0] for i in result]
        self.assert_(xs == sorted(xs))

    def test_z_perf(self):
        sd = L.star_detector(self.im640.size[0], self.im640.size[1])
        s = self.im640.tostring()
        niter = 100
        started = time.time()
        for i in range(niter):
            kp = sd.detect(s)
        took = time.time() - started
        print
        print
        print "%.1f ms/frame" % (1e3 * took / niter)
        print
        print


if __name__ == '__main__':
    if 0:
        suite = unittest.TestSuite()
        suite.addTest(TestSequenceFunctions('test_blank'))
        unittest.TextTestRunner(verbosity=2).run(suite)
    else:
        import rostest
        rostest.unitrun('star_detector', 'directed', TestDirected)
