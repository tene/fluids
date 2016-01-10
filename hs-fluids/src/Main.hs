module Main where
import qualified Control.Lens                       as Lens
import           Data.Array.Repa                    ((:.) (..), Array (..),
                                                     D (..), DIM2 (..), U (..))
import qualified Data.Array.Repa                    as Repa
import           Data.Array.Repa.Repr.Vector        (V (..))
import qualified Data.Array.Repa.Repr.Vector        as Repa.Vector
import           Data.Vector.Unboxed                (Vector (..), fromList)
import qualified Data.Vector.Unboxed                as Vector
import           Debug.Trace
import qualified Graphics.Gloss                     as Gloss
import qualified Graphics.Gloss.Interface.Pure.Game as Game
import           Graphics.Gloss.Raster.Array        (Color (..), rgb)
import qualified Graphics.Gloss.Raster.Array        as GRA

weights :: Vector Float
weights = fromList [
  4/9,
  1/9, 1/9, 1/9, 1/9,
  1/36, 1/36, 1/36, 1/36
  ]

directions :: Vector (Int, Int)
directions = fromList [
  (0,0),
  (1,0), (0,1), (-1,0), (0, -1),
  (1,1), (-1,1), (-1,-1), (1,-1)
  ]

floatDirections :: Vector (Float, Float)
floatDirections = Vector.map (Lens.over Lens.both fromIntegral) directions

rindex :: Vector Int
rindex = fromList [0,3,4,1,2,7,8,5,6]

indexes :: Vector Int
indexes = fromList [0..8]

type Fluid = Vector Float
type World = Array D DIM2 Fluid

dot2f :: (Float, Float) -> (Float, Float) -> Float
dot2f (a,b) (c,d) = a*c + b*d

-- Equilibrium distribution
-- fluidEq pressure velocity i = weights[i] * ( pressure + 3 * (velocities[i] `dot` velocity) - (3/2) * (velocity ^ 2) + (9/2) * ( ( velocities[i] `dot` velocity) ^ 2 ) )
-- fluidEq p u = map (\w v -> w * ( p + 3 * (v `dot` u) - (3/2) * u^2 + (9/2) * (v `dot` u)^2 ) ) (zip weights directions)
eqFluid :: (Float, Float) -> Float -> Fluid
eqFluid velocity pressure = Vector.map _compose_fd _pairs
  where
    _compose_fd (weight, direction) = weight * ( pressure + 3*(dot2f direction velocity) - (3/2)*(dot2f velocity velocity) + (9/2)*((dot2f direction velocity)**2))
    _pairs = Vector.zip weights floatDirections

eqFluid' = uncurry $ flip eqFluid

fluidPressure :: Fluid -> Float
fluidPressure f = p
  where
    (p, _) = fluidSummary f

fluidSummary :: Fluid -> (Float, (Float, Float))
fluidSummary f = Vector.foldl1' (\(p, (x,y)) (p', (x', y')) -> (p+p',(x+x'*p',y+y'*p'))) $ Vector.zip  f floatDirections

renderFluid :: Fluid -> Color
renderFluid f = rgb (p/10000) (p/10000) 1.0
  where p = fluidPressure f

initial :: World
initial = Repa.delay $ Repa.Vector.fromListVector (Repa.ix2 100 100) $ map (eqFluid (0,0)) [1..10000]

collide :: Float -> Fluid -> Fluid
collide v f = Vector.zipWith relax f equilibrium
  where
    equilibrium = eqFluid' $ fluidSummary f
    relax a b = (1-v)*a + v*b

viscosity :: Float
viscosity = 0.2

step :: World -> World
step world = Repa.traverse world id stream
  where
    stream lookup idx = collide viscosity $ Vector.map (fetchElem lookup idx) indexes

fetchElem :: (DIM2 -> Fluid) -> DIM2 -> Int -> Float
fetchElem lookup i j
  | Vector.null source = (Vector.!) source $ (Vector.unsafeIndex) rindex j
  | otherwise          = (Vector.!) cell $ (Vector.unsafeIndex) rindex j
  where
    offset = (uncurry Repa.ix2) $ (Vector.unsafeIndex) directions j
    sourcePos = Repa.addDim i offset
    source = lookup sourcePos
    cell = lookup i

renderField :: World -> Array D DIM2 Color
renderField f = Repa.traverse f id (\ _l _idx -> renderFluid $ _l _idx)

data Cell =
    Obstacle
  | Fluid
  -- | Empty
  -- | Fluid Double
  -- | Interface Double

handleInput :: Game.Event -> World -> World
handleInput (Game.EventKey (Game.SpecialKey Game.KeySpace) Game.Down _ _) w = trace "stepping" $ step w
handleInput _ w = w

main :: IO ()
main = do
  --GRA.animateArray (GRA.InWindow "Fluid Test" (400, 400) (10, 10)) (4,4) renderWorld
  GRA.playArray (GRA.InWindow "Fluid Test" (400, 400) (10, 10)) (4,4) 10 initial renderField handleInput (\_ w -> w)
  putStrLn "hello world"
