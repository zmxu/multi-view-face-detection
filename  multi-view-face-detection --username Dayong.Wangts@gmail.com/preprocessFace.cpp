const double DESIRED_LEFT_EYE_X = 0.16;     // Controls how much of the face is visible after preprocessing.
const double DESIRED_LEFT_EYE_Y = 0.14;
const double FACE_ELLIPSE_CY = 0.40;
const double FACE_ELLIPSE_W = 0.50;         // Should be atleast 0.5
const double FACE_ELLIPSE_H = 0.80;         // Controls how tall the face mask is.


#include "detectObject.h"       // Easily detect faces or eyes (using LBP or Haar Cascades).
#include "preprocessFace.h"     // Easily preprocess face images, for face recognition.

#include "ImageUtils.h"      // Shervin's handy OpenCV utility functions.

/*
// Remove the outer border of the face, so it doesn't include the background & hair.
// Keeps the center of the rectangle at the same place, rather than just dividing all values by 'scale'.
Rect scaleRectFromCenter(const Rect wholeFaceRect, float scale)
{
    float faceCenterX = wholeFaceRect.x + wholeFaceRect.width * 0.5f;
    float faceCenterY = wholeFaceRect.y + wholeFaceRect.height * 0.5f;
    float newWidth = wholeFaceRect.width * scale;
    float newHeight = wholeFaceRect.height * scale;
    Rect faceRect;
    faceRect.width = cvRound(newWidth);                        // Shrink the region
    faceRect.height = cvRound(newHeight);
    faceRect.x = cvRound(faceCenterX - newWidth * 0.5f);    // Move the region so that the center is still the same spot.
    faceRect.y = cvRound(faceCenterY - newHeight * 0.5f);
    
    return faceRect;
}
*/

// Search for both eyes within the given face image. Returns the eye centers in 'leftEye' and 'rightEye',
// or sets them to (-1,-1) if each eye was not found. Note that you can pass a 2nd eyeCascade if you
// want to search eyes using 2 different cascades. For example, you could use a regular eye detector
// as well as an eyeglasses detector, or a left eye detector as well as a right eye detector.
// Or if you don't want a 2nd eye detection, just pass an uninitialized CascadeClassifier.
// Can also store the searched left & right eye regions if desired.
void detectBothEyes(const Mat &face, CascadeClassifier &eyeCascade1, CascadeClassifier &eyeCascade2, Point &leftEye, Point &rightEye, Rect *searchedLeftEye, Rect *searchedRightEye)
{
    // Skip the borders of the face, since it is usually just hair and ears, that we don't care about.
/*
    // For "2splits.xml": Finds both eyes in roughly 60% of detected faces, also detects closed eyes.
    const float EYE_SX = 0.12f;
    const float EYE_SY = 0.17f;
    const float EYE_SW = 0.37f;
    const float EYE_SH = 0.36f;
*/
/*
    // For mcs.xml: Finds both eyes in roughly 80% of detected faces, also detects closed eyes.
    const float EYE_SX = 0.10f;
    const float EYE_SY = 0.19f;
    const float EYE_SW = 0.40f;
    const float EYE_SH = 0.36f;
*/

    // For default eye.xml or eyeglasses.xml: Finds both eyes in roughly 40% of detected faces, but does not detect closed eyes.
    const float EYE_SX = 0.16f;
    const float EYE_SY = 0.26f;
    const float EYE_SW = 0.30f;
    const float EYE_SH = 0.28f;

    int leftX = cvRound(face.cols * EYE_SX);
    int topY = cvRound(face.rows * EYE_SY);
    int widthX = cvRound(face.cols * EYE_SW);
    int heightY = cvRound(face.rows * EYE_SH);
    int rightX = cvRound(face.cols * (1.0-EYE_SX-EYE_SW) );  // Start of right-eye corner

    Mat topLeftOfFace = face(Rect(leftX, topY, widthX, heightY));
    Mat topRightOfFace = face(Rect(rightX, topY, widthX, heightY));
    Rect leftEyeRect, rightEyeRect;

    // Return the search windows to the caller, if desired.
    if (searchedLeftEye)
        *searchedLeftEye = Rect(leftX, topY, widthX, heightY);
    if (searchedRightEye)
        *searchedRightEye = Rect(rightX, topY, widthX, heightY);

    // Search the left region, then the right region using the 1st eye detector.
    detectLargestObject(topLeftOfFace, eyeCascade1, leftEyeRect, topLeftOfFace.cols);
    detectLargestObject(topRightOfFace, eyeCascade1, rightEyeRect, topRightOfFace.cols);

    // If the eye was not detected, try a different cascade classifier.
    if (leftEyeRect.width <= 0 && !eyeCascade2.empty()) {
        detectLargestObject(topLeftOfFace, eyeCascade2, leftEyeRect, topLeftOfFace.cols);
        //if (leftEyeRect.width > 0)
        //    cout << "2nd eye detector LEFT SUCCESS" << endl;
        //else
        //    cout << "2nd eye detector LEFT failed" << endl;
    }
    //else
    //    cout << "1st eye detector LEFT SUCCESS" << endl;

    // If the eye was not detected, try a different cascade classifier.
    if (rightEyeRect.width <= 0 && !eyeCascade2.empty()) {
        detectLargestObject(topRightOfFace, eyeCascade2, rightEyeRect, topRightOfFace.cols);
        //if (rightEyeRect.width > 0)
        //    cout << "2nd eye detector RIGHT SUCCESS" << endl;
        //else
        //    cout << "2nd eye detector RIGHT failed" << endl;
    }
    //else
    //    cout << "1st eye detector RIGHT SUCCESS" << endl;

    if (leftEyeRect.width > 0) {   // Check if the eye was detected.
        leftEyeRect.x += leftX;    // Adjust the left-eye rectangle because the face border was removed.
        leftEyeRect.y += topY;
        leftEye = Point(leftEyeRect.x + leftEyeRect.width/2, leftEyeRect.y + leftEyeRect.height/2);
    }
    else {
        leftEye = Point(-1, -1);    // Return an invalid point
    }

    if (rightEyeRect.width > 0) { // Check if the eye was detected.
        rightEyeRect.x += rightX; // Adjust the right-eye rectangle, since it starts on the right side of the image.
        rightEyeRect.y += topY;  // Adjust the right-eye rectangle because the face border was removed.
        rightEye = Point(rightEyeRect.x + rightEyeRect.width/2, rightEyeRect.y + rightEyeRect.height/2);
    }
    else {
        rightEye = Point(-1, -1);    // Return an invalid point
    }
}

// Histogram Equalize seperately for the left and right sides of the face.
void equalizeLeftAndRightHalves(Mat &faceImg)
{
    // It is common that there is stronger light from one half of the face than the other. In that case,
    // if you simply did histogram equalization on the whole face then it would make one half dark and
    // one half bright. So we will do histogram equalization separately on each face half, so they will
    // both look similar on average. But this would cause a sharp edge in the middle of the face, because
    // the left half and right half would be suddenly different. So we also histogram equalize the whole
    // image, and in the middle part we blend the 3 images together for a smooth brightness transition.

    int w = faceImg.cols;
    int h = faceImg.rows;

    // 1) First, equalize the whole face.
    Mat wholeFace;
    equalizeHist(faceImg, wholeFace);

    // 2) Equalize the left half and the right half of the face separately.
    int midX = w/2;
    Mat leftSide = faceImg(Rect(0,0, midX,h));
    Mat rightSide = faceImg(Rect(midX,0, w-midX,h));
    equalizeHist(leftSide, leftSide);
    equalizeHist(rightSide, rightSide);

    // 3) Combine the left half and right half and whole face together, so that it has a smooth transition.
    for (int y=0; y<h; y++) {
        for (int x=0; x<w; x++) {
            int v;
            if (x < w/4) {          // Left 25%: just use the left face.
                v = leftSide.at<uchar>(y,x);
            }
            else if (x < w*2/4) {   // Mid-left 25%: blend the left face & whole face.
                int lv = leftSide.at<uchar>(y,x);
                int wv = wholeFace.at<uchar>(y,x);
                // Blend more of the whole face as it moves further right along the face.
                float f = (x - w*1/4) / (float)(w*0.25f);
                v = cvRound((1.0f - f) * lv + (f) * wv);
            }
            else if (x < w*3/4) {   // Mid-right 25%: blend the right face & whole face.
                int rv = rightSide.at<uchar>(y,x-midX);
                int wv = wholeFace.at<uchar>(y,x);
                // Blend more of the right-side face as it moves further right along the face.
                float f = (x - w*2/4) / (float)(w*0.25f);
                v = cvRound((1.0f - f) * wv + (f) * rv);
            }
            else {                  // Right 25%: just use the right face.
                v = rightSide.at<uchar>(y,x-midX);
            }
            faceImg.at<uchar>(y,x) = v;
        }// end x loop
    }//end y loop
}


// Create a grayscale face image that has a standard size and contrast & brightness.
// "srcImg" should be a copy of the whole color camera frame, so that it can draw the eye positions onto.
// If 'doLeftAndRightSeparately' is true, it will process left & right sides seperately,
// so that if there is a strong light on one side but not the other, it will still look OK.
// Performs Face Preprocessing as a combination of:
//  - geometrical scaling, rotation and translation using Eye Detection,
//  - smoothing away image noise using a Bilateral Filter,
//  - standardize the brightness on both left and right sides of the face independently using separated Histogram Equalization,
//  - removal of background and hair using an Elliptical Mask.
// Returns either a preprocessed face square image or NULL (ie: couldn't detect the face and 2 eyes).
// If a face is found, it can store the rect coordinates into 'storeFaceRect' and 'storeLeftEye' & 'storeRightEye' if given,
// and eye search regions into 'searchedLeftEye' & 'searchedRightEye' if given.
Mat getPreprocessedFace(Mat &srcImg, int desiredFaceWidth, CascadeClassifier &faceCascade, CascadeClassifier &eyeCascade1, CascadeClassifier &eyeCascade2, bool doLeftAndRightSeparately, Rect *storeFaceRect, Point *storeLeftEye, Point *storeRightEye, Rect *searchedLeftEye, Rect *searchedRightEye)
{
    // Use square faces.
    int desiredFaceHeight = desiredFaceWidth;

    // Mark the detected face region and eye search regions as invalid, in case they aren't detected.
    if (storeFaceRect)
        storeFaceRect->width = -1;
    if (storeLeftEye)
        storeLeftEye->x = -1;
    if (storeRightEye)
        storeRightEye->x= -1;
    if (searchedLeftEye)
        searchedLeftEye->width = -1;
    if (searchedRightEye)
        searchedRightEye->width = -1;

    // Find the largest face.
    Rect faceRect;
    detectLargestObject(srcImg, faceCascade, faceRect,SCALEDWIDTH);

    // Check if a face was detected.
    if (faceRect.width > 0) {

        // Give the face rect to the caller if desired.
        if (storeFaceRect)
            *storeFaceRect = faceRect;

        Mat faceImg = srcImg(faceRect);    // Get the detected face image.

        // If the input image is not grayscale, then convert the BGR or BGRA color image to grayscale.
        Mat gray;
        if (faceImg.channels() == 3) {
            cvtColor(faceImg, gray, CV_BGR2GRAY);
        }
        else if (faceImg.channels() == 4) {
            cvtColor(faceImg, gray, CV_BGRA2GRAY);
        }
        else {
            // Access the input image directly, since it is already grayscale.
            gray = faceImg;
        }

        // Search for the 2 eyes at the full resolution, since eye detection needs max resolution possible!
        Point leftEye, rightEye;
        detectBothEyes(gray, eyeCascade1, eyeCascade2, leftEye, rightEye, searchedLeftEye, searchedRightEye);

        // Give the eye results to the caller if desired.
        if (storeLeftEye)
            *storeLeftEye = leftEye;
        if (storeRightEye)
            *storeRightEye = rightEye;

        // Check if both eyes were detected.
        if (leftEye.x >= 0 && rightEye.x >= 0) {

            // Make the face image the same size as the training images.

            // Since we found both eyes, lets rotate & scale & translate the face so that the 2 eyes
            // line up perfectly with ideal eye positions. This makes sure that eyes will be horizontal,
            // and not too far left or right of the face, etc.

            // Get the center between the 2 eyes.
            Point2f eyesCenter = Point2f( (leftEye.x + rightEye.x) * 0.5f, (leftEye.y + rightEye.y) * 0.5f );
            // Get the angle between the 2 eyes.
            double dy = (rightEye.y - leftEye.y);
            double dx = (rightEye.x - leftEye.x);
            double len = sqrt(dx*dx + dy*dy);
            double angle = atan2(dy, dx) * 180.0/CV_PI; // Convert from radians to degrees.

            // Hand measurements shown that the left eye center should ideally be at roughly (0.19, 0.14) of a scaled face image.
            const double DESIRED_RIGHT_EYE_X = (1.0f - DESIRED_LEFT_EYE_X);
            // Get the amount we need to scale the image to be the desired fixed size we want.
            double desiredLen = (DESIRED_RIGHT_EYE_X - DESIRED_LEFT_EYE_X) * desiredFaceWidth;
            double scale = desiredLen / len;
            // Get the transformation matrix for rotating and scaling the face to the desired angle & size.
            Mat rot_mat = getRotationMatrix2D(eyesCenter, angle, scale);
            // Shift the center of the eyes to be the desired center between the eyes.
            rot_mat.at<double>(0, 2) += desiredFaceWidth * 0.5f - eyesCenter.x;
            rot_mat.at<double>(1, 2) += desiredFaceHeight * DESIRED_LEFT_EYE_Y - eyesCenter.y;

            // Rotate and scale and translate the image to the desired angle & size & position!
            // Note that we use 'w' for the height instead of 'h', because the input face has 1:1 aspect ratio.
            Mat warped = Mat(desiredFaceHeight, desiredFaceWidth, CV_8U, Scalar(128)); // Clear the output image to a default grey.
            warpAffine(gray, warped, rot_mat, warped.size());
            //imshow("warped", warped);

            // Give the image a standard brightness and contrast, in case it was too dark or had low contrast.
            if (!doLeftAndRightSeparately) {
                // Do it on the whole face.
                equalizeHist(warped, warped);
            }
            else {
                // Do it seperately for the left and right sides of the face.
                equalizeLeftAndRightHalves(warped);
            }
            //imshow("equalized", warped);

            // Use the "Bilateral Filter" to reduce pixel noise by smoothing the image, but keeping the sharp edges in the face.
            Mat filtered = Mat(warped.size(), CV_8U);
            bilateralFilter(warped, filtered, 0, 20.0, 2.0);
            //imshow("filtered", filtered);

            // Filter out the corners of the face, since we mainly just care about the middle parts.
            // Draw a filled ellipse in the middle of the face-sized image.
            Mat mask = Mat(warped.size(), CV_8U, Scalar(0)); // Start with an empty mask.
            Point faceCenter = Point( desiredFaceWidth/2, cvRound(desiredFaceHeight * FACE_ELLIPSE_CY) );
            Size size = Size( cvRound(desiredFaceWidth * FACE_ELLIPSE_W), cvRound(desiredFaceHeight * FACE_ELLIPSE_H) );
            ellipse(mask, faceCenter, size, 0, 0, 360, Scalar(255), CV_FILLED);
            //imshow("mask", mask);

            // Use the mask, to remove outside pixels.
            Mat dstImg = Mat(warped.size(), CV_8U, Scalar(128)); // Clear the output image to a default gray.
            /*
            namedWindow("filtered");
            imshow("filtered", filtered);
            namedWindow("dstImg");
            imshow("dstImg", dstImg);
            namedWindow("mask");
            imshow("mask", mask);
            */
            // Apply the elliptical mask on the face.
            filtered.copyTo(dstImg, mask);  // Copies non-masked pixels from filtered to dstImg.
            //imshow("dstImg", dstImg);

            return dstImg;
        }
        /*
        else {
            // Since no eyes were found, just do a generic image resize.
            resize(gray, tmpImg, Size(w,h));
        }
        */
    }
    return Mat();
}

int multiViewFaceDetection(Mat &srcImg, CascadeClassifier &faceCascade, CascadeClassifier &faceCascade2, vector<FacePositionInfo>& storeFpi)
{
	// If the input image is not grayscale, then convert the BGR or BGRA color image to grayscale.
	int facetype = -1;
    Mat srcImgGray;
    if (srcImg.channels() == 3) {
        cvtColor(srcImg, srcImgGray, CV_BGR2GRAY);
    }
    else if (srcImg.channels() == 4) {
        cvtColor(srcImg, srcImgGray, CV_BGRA2GRAY);
    }
    else {
        // Access the input image directly, since it is already grayscale.
        srcImgGray = srcImg;
    }	
	//preprocess
    // Possibly shrink the image, to run much faster.
	int scaledWidth = SCALEDWIDTH;
    Mat inputImg;
    float scale = srcImgGray.cols / (float)scaledWidth;
    if (srcImgGray.cols > scaledWidth) {
        // Shrink the image while keeping the same aspect ratio.
        int scaledHeight = cvRound(srcImgGray.rows / scale);
        resize(srcImgGray, inputImg, Size(scaledWidth, scaledHeight));
    }
    else {
        // Access the input image directly, since it is already small.
        inputImg = srcImgGray;
    }
    // Standardize the brightness and contrast to improve dark images.
    Mat equalizedImg;
    equalizeHist(inputImg, equalizedImg);
	

	Rect faceRect;
	FacePositionInfo tmpfpi;
	// Find the largest face.
	int findface = 0;
#ifdef _DEBUG
	namedWindow("Debug");
#endif
	if(!findface){
		detectLargestObject_afterprocess(equalizedImg, scale, srcImgGray.cols, srcImgGray.rows, faceCascade, faceRect, scaledWidth);
		// Check if a face was detected.		
		if (faceRect.width > 0) {
			rect2FPI(faceRect,tmpfpi,0,srcImgGray.cols,srcImgGray.rows);
			storeFpi.push_back(tmpfpi);
			findface = 1;
			facetype = 1; // front face
		}
	}
	if(!findface){
		//turn to right
		detectLargestObject_afterprocess(equalizedImg, scale, srcImgGray.cols, srcImgGray.rows, faceCascade2, faceRect, scaledWidth);
		// Check if a face was detected.		
		if (faceRect.width > 0) {
#ifdef _DEBUG
			Rect faceRect2;
			faceRect2.x = cvRound(faceRect.x / scale);
            faceRect2.y = cvRound(faceRect.y / scale);
            faceRect2.width = cvRound(faceRect.width / scale);
            faceRect2.height = cvRound(faceRect.height / scale);
			rectangle(equalizedImg,faceRect2,CV_RGB(255,0,0),2,CV_AA);
			imshow("DEBUG",equalizedImg);
#endif
#ifdef _DEBUG
			printf("@@@@Find Profile Face -- RIGHT\n");
#endif
			rect2FPI(faceRect,tmpfpi,0,srcImgGray.cols,srcImgGray.rows);
			storeFpi.push_back(tmpfpi);
			findface = 1;
			facetype = 2; // right face
		}		
		//turn to left
		if(!findface){
			Mat equalizedImg_flip;
			flip(equalizedImg,equalizedImg_flip,1);
			detectLargestObject_afterprocess(equalizedImg_flip, scale, srcImgGray.cols, srcImgGray.rows, faceCascade2, faceRect, scaledWidth);
			// Check if a face was detected.		
			if (faceRect.width > 0) {
#ifdef _DEBUG
			Rect faceRect2;
			faceRect2.x = cvRound(faceRect.x / scale);
            faceRect2.y = cvRound(faceRect.y / scale);
            faceRect2.width = cvRound(faceRect.width / scale);
            faceRect2.height = cvRound(faceRect.height / scale);
			rectangle(equalizedImg_flip,faceRect2,CV_RGB(255,0,0),2,CV_AA);
			imshow("DEBUG",equalizedImg_flip);
#endif
				faceRect.x = cvRound(faceRect.x / scale);
				faceRect.width = cvRound(faceRect.width / scale);
				faceRect.x = equalizedImg_flip.cols - (faceRect.x + faceRect.width);				
				faceRect.x = cvRound(faceRect.x * scale);
				faceRect.width = cvRound(faceRect.width * scale);
#ifdef _DEBUG
				printf("@@@@Find Profile Face -- LEFT\n");
#endif
				rect2FPI(faceRect,tmpfpi,0,srcImgGray.cols,srcImgGray.rows);
				storeFpi.push_back(tmpfpi);
				findface = 1;
				facetype = 3; // left face
			}
		}
	}
	/*
	if(!findface){
		IplImage iplInput = equalizedImg;
		Mat equalizedImgRotated;	
		//equalizedImg.copyTo(equalizedImgRotated);
		IplImage iplInputRotated = equalizedImgRotated;
		int anglelist[] = {45,-45};
		int count = 0;
		while(!findface){
			if(count == sizeof(anglelist)/sizeof(int))
				break;		
			//GS_rotate(&iplInput, &iplInputRotated, anglelist[count]);    	
			rotateImageMat(equalizedImg,equalizedImgRotated,anglelist[count]);
#ifdef _DEBUG
			imshow( "DEBUG", equalizedImgRotated); 
#endif
			detectLargestObject_afterprocess(equalizedImgRotated, scale, srcImgGray.cols, srcImgGray.rows, faceCascade, faceRect,scaledWidth);		
			// Check if a face was detected.
			if (faceRect.width > 0) {
#ifdef _DEBUG
				printf("****Find Rotated Face\n");
#endif
				rect2FPI(faceRect,tmpfpi,anglelist[count],srcImgGray.cols,srcImgGray.rows);
				storeFpi.push_back(tmpfpi);
				findface = 1;
				facetype = 4 + count; // 4 -> 45' 5 -> -45'
			}
			count++;
		}
	}
	*/
    return facetype;
}

void rect2FPI(Rect& faceRect, FacePositionInfo& fpi, int angle, int width, int height){
	double radius = (-angle)*(M_PI/180.0);
    double sin_value = sin(radius);
    double cos_value = cos(radius);
    int centerX = height/2;
    int centerY = width/2;
	fpi.y1 = ((faceRect.y-centerX)*cos_value - (faceRect.x-centerY)*sin_value + centerX);
	fpi.x1 = ((faceRect.y-centerX)*sin_value + (faceRect.x-centerY)*cos_value + centerY);
	fpi.y2 = ((faceRect.y-centerX)*cos_value - (faceRect.x+faceRect.width-centerY)*sin_value + centerX);
	fpi.x2 = ((faceRect.y-centerX)*sin_value + (faceRect.x+faceRect.width-centerY)*cos_value + centerY);
	fpi.y3 = ((faceRect.y+faceRect.height-centerX)*cos_value - (faceRect.x+faceRect.width-centerY)*sin_value + centerX);
	fpi.x3 = ((faceRect.y+faceRect.height-centerX)*sin_value + (faceRect.x+faceRect.width-centerY)*cos_value + centerY);
	fpi.y4 = ((faceRect.y+faceRect.height-centerX)*cos_value - (faceRect.x-centerY)*sin_value + centerX);
	fpi.x4 = ((faceRect.y+faceRect.height-centerX)*sin_value + (faceRect.x-centerY)*cos_value + centerY); 	
}

void GS_rotate( IplImage* srcImg, IplImage *srcImgRotated, int angle )
{
    int i, j;
    double new_x, new_y, var;    

    int height = srcImg->height;
    int width  = srcImg->width;
    
	double radius = -angle*(M_PI/180.0);
    double sin_value = sin(radius);
    double cos_value = cos(radius);

    int centerX = height/2;
    int centerY = width/2;

    for(i=0; i<height; i++)
    {
        for(j=0; j<width; j++)
        {
            new_x = (i-centerX)*cos_value - (j-centerY)*sin_value + centerX;
            new_y = (i-centerX)*sin_value + (j-centerY)*cos_value + centerY;

            if(new_x < 0 || new_x > height) 
            {
                var = 0.0; 
            } else if(new_y < 0 || new_y > width) {
                var = 0.0; 
            } else {
                var = cvGetReal2D( srcImg, (int)new_x, (int)new_y );
            }

            cvSetReal2D( srcImgRotated, i, j, var );
        }
    }    
}