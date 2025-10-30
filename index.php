<?php
// Old way: https://localhost/index.php?echo=true&img=false&imgDataOutputOnly=false
// Use this: https://localhost/index.php?echo&img
$echoOn = false;
$imgOn = false;
//$imgDataOutputOnly = true;
$info = false;
$source = 0;
$width = -1;
$height = -1;


$webPageStart = '
<!DOCTYPE html>
<html>
    <head>
        <title>PHP Test FooBar</title>
    </head>
    <body style="font-family: Arial, sans-serif; margin: 20px;">
        <h1>PHP Image Processing Test</h1>';

$webPageEnd = '
    </body>
</html>';

function logmsg($data) {
    $output = $data;
    if (is_array($output))
        $output = implode(',', $output);
    echo "<script>console.log('Debug Objects: " . $output . "' );</script>";
}

if (count($_GET) > 0) {
    // Shorter version, check if the parameter exists
    if (isset($_GET['echo'])) {
        $echoOn = true;
    }
    if (isset($_GET['img'])) {
        $imgOn = true;
    }
    if (isset($_GET['info'])) {
        $info = true;
    }
    if (isset($_GET['source'])) {  
        $source = intval($_GET['source']);
    }
    if (isset($_GET['width'])) {
        $width = intval($_GET['width']);
    }
    if (isset($_GET['height'])) {
        $height = intval($_GET['height']);
    }    
}

if ($info) {
    $echoOn = true;
    $imgOn = true;
}

if ($info) echo $webPageStart;

logmsg("Info is " . ($info ? 'ON' : 'OFF'));    
logmsg("Echo is " . ($echoOn ? 'ON' : 'OFF'));
logmsg("Image display is " . ($imgOn ? 'ON' : 'OFF'));  
logmsg("width is " . $width);
logmsg("height is " . $height);

// Enhanced image download function with better redirect handling
function downloadImageWithRedirects($url, $maxRedirects = 10, $timeout = 30) {
    $ch = curl_init();
   
    // Basic cURL options
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
    curl_setopt($ch, CURLOPT_MAXREDIRS, $maxRedirects);
    curl_setopt($ch, CURLOPT_TIMEOUT, $timeout);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
    curl_setopt($ch, CURLOPT_USERAGENT, 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36');
   
    // SSL options for HTTPS
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
   
    // Additional headers that some services expect
    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        'Accept: image/webp,image/apng,image/*,*/*;q=0.8',
        'Accept-Language: en-US,en;q=0.9',
        'Accept-Encoding: gzip, deflate',
        'Connection: keep-alive',
        'Upgrade-Insecure-Requests: 1'
    ]);
   
    // Enable verbose output for debugging (remove in production)
    // curl_setopt($ch, CURLOPT_VERBOSE, true);
   
    $imageData = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $finalUrl = curl_getinfo($ch, CURLINFO_EFFECTIVE_URL);
    $redirectCount = curl_getinfo($ch, CURLINFO_REDIRECT_COUNT);
    $totalTime = curl_getinfo($ch, CURLINFO_TOTAL_TIME);
   
    curl_close($ch);
   
    return [
        'data' => $imageData,
        'http_code' => $httpCode,
        'final_url' => $finalUrl,
        'redirect_count' => $redirectCount,
        'total_time' => $totalTime,
        'success' => ($imageData !== false && $httpCode == 200)
    ];
}

// Function to standardize JPEG format
function standardizeJpeg($imageData, $quality = 90) {
    // Create image resource from any supported format
    $sourceImage = imagecreatefromstring($imageData);
   
    if ($sourceImage === false) {
        return false;
    }
   
    // Get dimensions
    $width = imagesx($sourceImage);
    $height = imagesy($sourceImage);
   
    // Create a new true color image (24-bit RGB)
    $standardImage = imagecreatetruecolor($width, $height);
   
    // Ensure high quality resampling
    imagealphablending($standardImage, false);
    imagesavealpha($standardImage, true);
   
    // Copy the image to ensure standard format
    imagecopyresampled(
        $standardImage, $sourceImage,
        0, 0, 0, 0,
        $width, $height, $width, $height
    );
   
    // Output as standard baseline JPEG
    ob_start();
    imagejpeg($standardImage, null, $quality);
    $standardImageData = ob_get_contents();
    ob_end_clean();
   
    // Clean up
    imagedestroy($sourceImage);
    imagedestroy($standardImage);
   
    return $standardImageData;
}

// Image resizing function with standardization
function resizeImageWithCrop($imageData, $targetWidth, $targetHeight, $quality = 90) {
    // First standardize the input image
    $standardImageData = standardizeJpeg($imageData, 95); // Higher quality for intermediate processing
    if ($standardImageData === false) {
        return false;
    }
   
    // Create image resource from standardized data
    $sourceImage = imagecreatefromstring($standardImageData);
   
    if ($sourceImage === false) {
        return false;
    }
   
    // Get original dimensions
    $originalWidth = imagesx($sourceImage);
    $originalHeight = imagesy($sourceImage);
   
    // Calculate aspect ratios
    $originalRatio = $originalWidth / $originalHeight;
    $targetRatio = $targetWidth / $targetHeight;
   
    // Determine crop dimensions
    if ($originalRatio > $targetRatio) {
        // Original is wider - crop width
        $cropHeight = $originalHeight;
        $cropWidth = $originalHeight * $targetRatio;
        $cropX = ($originalWidth - $cropWidth) / 2; // Center horizontally
        $cropY = 0;
    } else {
        // Original is taller - crop height
        $cropWidth = $originalWidth;
        $cropHeight = $originalWidth / $targetRatio;
        $cropX = 0;
        $cropY = ($originalHeight - $cropHeight) / 2; // Center vertically
    }
   
    // Create new image with target dimensions
    $resizedImage = imagecreatetruecolor($targetWidth, $targetHeight);
   
    // Set high quality resampling
    imagealphablending($resizedImage, false);
    imagesavealpha($resizedImage, true);
   
    // Copy and resize the cropped portion
    imagecopyresampled(
        $resizedImage,      // Destination image
        $sourceImage,       // Source image
        0, 0,              // Destination x, y
        $cropX, $cropY,    // Source x, y (crop start)
        $targetWidth, $targetHeight,  // Destination width, height
        $cropWidth, $cropHeight       // Source width, height (crop size)
    );
   
    // Convert back to standardized JPEG
    ob_start();
    imagejpeg($resizedImage, null, $quality);
    $resizedImageData = ob_get_contents();
    ob_end_clean();
   
    // Clean up memory
    imagedestroy($sourceImage);
    imagedestroy($resizedImage);
   
    return $resizedImageData;
}

// Define multiple image sources
$imageSources = [
    '0' => 'https://picsum.photos/', // Small random image
    '1' => 'https://c02.purpledshub.com/uploads/sites/40/2023/08/JI230816Cosmos220-6d9254f-edited-scaled.jpg',
    '2' => 'https://picsum.photos/800/600', // Random image 800x600
    '3' => 'https://picsum.photos/id/237/800/600' // Specific image (dog)    
];

// Select image source (you can make this dynamic via GET parameter)
$selectedSource = isset($_GET['source']) ? $_GET['source'] : '0';
$suffix = '';

if ($selectedSource == '0') {
    logmsg('>>> Selecting suffix...');
    // Build URL with specific dimensions for picsum.photos
    if (($height > -1 && $width > -1) && ($height == $width)) {
       logmsg(('>>>>> (1)'));
       $suffix = $width;
    } else if ($width > -1 && $height == -1) {
        logmsg(('>>>>> ( 2)'));
        $height = $width;
        $suffix = $width;
    } else if ($height > -1 && $width > -1) {
        logmsg(('>>>>> (3)'));
        $suffix = $width . '/' . $height;
    } else if ($height == -1 && $width == -1) {
        logmsg(('>>>>> (4)'));
        $suffix = '128'; // Default size
    }
    else {
        logmsg(('>>>>> (5)'));
    }
    $imageUrl = $imageSources['0'] . $suffix; // Request a specific size
} else {
    $imageUrl = isset($imageSources[$selectedSource]) ? $imageSources[$selectedSource] : $imageSources['0'];
}

logmsg(' ----- ');
logmsg("suffix set to: " . $suffix);
logmsg("Selected source: " . $selectedSource);
logmsg("Image URL: " . $imageUrl);

//$imageUrl = isset($imageSources[$selectedSource]) ? $imageSources[$selectedSource] : $imageSources['0'];

if ($echoOn) echo "<p>Using image source: <strong>$selectedSource</strong></p>";
if ($echoOn) echo "<p>Image URL: <strong>$imageUrl</strong></p>";

// Download image using enhanced function
$downloadResult = downloadImageWithRedirects($imageUrl, 10, 30);

if ($downloadResult['success']) {
    $imageData = $downloadResult['data'];
   
    if ($echoOn) echo '<p>Image downloaded successfully!</p>';
    if ($echoOn) echo '<p>Final URL: ' . $downloadResult['final_url'] . '</p>';
    if ($echoOn) echo '<p>Redirects: ' . $downloadResult['redirect_count'] . '</p>';
    if ($echoOn) echo '<p>Download time: ' . round($downloadResult['total_time'], 2) . 's</p>';
    if ($echoOn) echo '<p>Original image size: ' . strlen($imageData) . ' bytes</p>';
   
    // Standardize the original image
    $standardizedImage = standardizeJpeg($imageData, 90);
    if ($echoOn) echo '<p>Standardized image size: ' . strlen($standardizedImage) . ' bytes</p>';
   
    // Display original image
    $base64Original = base64_encode($imageData);
    if ($echoOn) echo '<h3>Original Image:</h3>';
    if ($imgOn) echo '<img src="data:image/jpeg;base64,' . $base64Original . '" style="max-width: 300px;" alt="Original Image">';
   
    // Display standardized image
    $base64Standardized = base64_encode($standardizedImage);
    if ($echoOn) echo '<h3>Standardized Image (Baseline JPEG, 24-bit):</h3>';
    if ($imgOn) echo '<img src="data:image/jpeg;base64,' . $base64Standardized . '" style="max-width: 300px;" alt="Standardized Image">';
   
    // Resize image based on provided width and height (default 128x128)
    $resizedImage = resizeImageWithCrop($imageData, $width, $height, 100);
    if ($resizedImage) {
        if ($echoOn) echo '<h3>Resized Image (' . $width . 'x' . $height . '):</h3>';
        $base64Resized4 = base64_encode($resizedImage);
        if ($imgOn) echo '<img src="data:image/jpeg;base64,' . $base64Resized4 . '" alt="Resized Image" style="max-width: 300px;">';
        if ($echoOn) echo '<p>Resized image size: ' . strlen($resizedImage) . ' bytes</p>';
    }

    if ($echoOn) {        
        echo '<h3>Base64 Resized Image Data (' . $width . 'x' . $height . '):</h3>';
        echo '<textarea style="width:100%; height:200px;">' . $base64Resized4 . '</textarea>';
    }    

    if (!$info) {
        // Output only the last resized image data
        echo $base64Resized4;
    }  
   
} else {
    if ($echoOn) echo '<p>Failed to download image! HTTP Code: ' . $downloadResult['http_code'] . '</p>';
    if ($echoOn) echo '<p>Final URL: ' . $downloadResult['final_url'] . '</p>';
    if ($echoOn) echo '<p>Redirects: ' . $downloadResult['redirect_count'] . '</p>';
}

if ($info) echo $webPageEnd;
?>
