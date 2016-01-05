module Main where
import           Data.Array.Repa as Repa
import           Graphics.Gloss

weights :: [Double]
weights = [
  4/9,
  1/9, 1/9, 1/9, 1/9,
  1/36, 1/36, 1/36, 1/36
  ]

velocities :: [(Int, Int)]
velocities = [
  (0,0),
  (1,0), (0,1), (-1,0), (0, -1),
  (1,1), (-1,1), (-1,-1), (1,-1)
  ]

rindex :: [Int]
rindex = [0,3,4,1,2,7,8,5,6]

type Fluid = Double

renderFluid :: Fluid -> Picture
renderFluid f = pictures [text $ show (floor f), lineLoop [(0,0),(100,0),(100,100),(0,100)]]

initial :: Array U DIM2 Fluid
initial = fromListUnboxed (ix2 5 20) [1..100]

renderField :: Array U DIM2 Fluid -> Picture
renderField f = pictures $ toList f'
  where
    f' = Repa.traverse f id _render
    _render _lookup idx = _move idx $ renderFluid $ _lookup idx
    _move (Z :. x :. y) = translate (fromIntegral x*100) (fromIntegral y*100)

data Cell =
    Obstacle
  | Fluid
  -- | Empty
  -- | Fluid Double
  -- | Interface Double

renderWorld :: a -> Picture
renderWorld _ = pictures [text "Hello", lineLoop [(0,0),(100,0),(100,100),(0,100)]]

main :: IO ()
main = do
  display (InWindow "Fluid Test" (20, 20) (10, 10)) white $ renderField initial
  putStrLn "hello world"
