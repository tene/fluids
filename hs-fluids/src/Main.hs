{-# LANGUAGE BangPatterns        #-}
{-# LANGUAGE ScopedTypeVariables #-}
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
import           System.Exit

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
type World = Array V DIM2 Fluid

dot2f :: (Float, Float) -> (Float, Float) -> Float
dot2f (!a,!b) (!c,!d) = a*c + b*d

-- Equilibrium distribution
-- fluidEq pressure velocity i = weights[i] * ( pressure + 3 * (velocities[i] `dot` velocity) - (3/2) * (velocity ^ 2) + (9/2) * ( ( velocities[i] `dot` velocity) ^ 2 ) )
-- fluidEq p u = map (\w v -> w * ( p + 3 * (v `dot` u) - (3/2) * u^2 + (9/2) * (v `dot` u)^2 ) ) (zip weights directions)
eqFluid :: (Float, Float) -> Float -> Fluid
eqFluid !velocity !pressure = Vector.map _compose_fd _pairs
  where
    _compose_fd (!weight, !direction) = weight * ( pressure + 3*(dot2f direction velocity) - (3/2)*(dot2f velocity velocity) + (9/2)*((dot2f direction velocity)**2))
    !_pairs = Vector.zip weights floatDirections

eqFluid' = uncurry $ flip eqFluid

fluidPressure :: Fluid -> Float
fluidPressure !f = Vector.sum f

fluidVelocity :: Fluid -> (Float, Float)
--fluidVelocity !f = Vector.foldl' (\(!x,!y) (!p', (!x', !y')) -> (x+x'*p',y+y'*p')) (0,0) $ Vector.zip  f floatDirections
--fluidVelocity !f = Vector.foldl1' (\(!x,!y) (!x', !y') -> (x+x',y+y')) $ Vector.zipWith (\p (x,y) -> (x*p,y*p))  f floatDirections
fluidVelocity !f = go 9 0 0 (Vector.toList f) (Vector.toList floatDirections)
  where
    go 0 !x !y _ _ = (x,y)
    go !n !x !y (!p:ps) ((!x',!y'):dirs) = go (n-1) (p*x') (p*y') ps dirs

fluidSummary :: Fluid -> (Float, (Float, Float))
fluidSummary !f = Vector.foldl1' (\(!p, (!x,!y)) (!p', (!x', !y')) -> (p+p',(x+x'*p',y+y'*p'))) $ Vector.zip  f floatDirections

initial :: World
initial = Repa.Vector.fromListVector (Repa.ix2 20 20) $ map (\p -> eqFluid (0,0) (p/400)) [1..400]

collide :: Float -> Fluid -> Fluid
collide v !f = Vector.zipWith relax f equilibrium
  where
    equilibrium = eqFluid (fluidVelocity f) (fluidPressure f)
    relax !a !b = (1-v)*a + v*b

viscosity :: Float
viscosity = 0.5

step :: World -> IO World
step !world = return $ Repa.computeS $ Repa.traverse world id stream
  where
    stream _ ((Repa.Z :. 0) :. _) = eqFluid (0,0.1) 1.0
    stream !lookup !idx = collide viscosity $ Vector.map (fetchElem (Repa.extent world) lookup idx) indexes

fetchElem :: DIM2 -> (DIM2 -> Fluid) -> DIM2 -> Int -> Float
fetchElem bounds lookup i j
  | Repa.inShape bounds sourcePos = (Vector.!) source j
  | otherwise                     = (Vector.!) cell $ Vector.unsafeIndex rindex j
  where
    (!ox, !oy) = Vector.unsafeIndex directions $ Vector.unsafeIndex rindex j
    !offset = Repa.ix2 ox oy
    !sourcePos = Repa.addDim i offset
    source = lookup sourcePos
    cell = lookup i

renderFluid :: Float -> Float -> Color
renderFluid !m !p = rgb (p/m) (p/m) 1.0

renderField :: World -> IO (Array D DIM2 Color)
renderField !f = do
  -- (pressureField :: Array U DIM2 Float) <- Repa.computeP $ Repa.map fluidPressure f
  let (pressureField :: Array U DIM2 Float) = Repa.computeS $ Repa.map fluidPressure f
      maxPressure = Repa.foldAllS max 0 pressureField
  return $ Repa.traverse pressureField id (\ _l _idx -> (renderFluid maxPressure) $ _l _idx)

data Cell =
    Obstacle
  | Fluid
  -- | Empty
  -- | Fluid Double
  -- | Interface Double

handleInput :: Game.Event -> World -> IO World
-- handleInput (Game.EventKey (Game.SpecialKey Game.KeySpace) Game.Down _ _) w = trace "stepping" $ step w
handleInput (Game.EventKey (Game.SpecialKey Game.KeyEsc) Game.Down _ _) _ = exitSuccess
-- handleInput e w = traceShowM e >> return w
handleInput _ w = return w

runN 0 a _ = return a
runN n a f = do
  putStrLn "step"
  b <- f a
  runN (n-1) b f

main :: IO ()
main = do
  --GRA.animateArray (GRA.InWindow "Fluid Test" (400, 400) (10, 10)) (4,4) renderWorld
  GRA.playArrayIO (GRA.InWindow "Fluid Test" (400, 400) (10, 10)) (40,40) 100 initial renderField handleInput (\_ w -> step w)
  -- putStrLn "starting"
  -- world <- runN 100 initial step
  -- print $ Repa.index world (Repa.ix2 0 0)
  --print $ fluidSummary $ eqFluid (0,0) 0.2
